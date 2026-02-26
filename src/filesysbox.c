/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

/****** filesysbox.library/--about-handlers-- *******************************
*
*   A filesysbox handler leaves all the work of managing
*   locks, notifications, file handles, packets, update
*   timeouts, etc. to the filesysbox.library.
*
****************************************************************************/

/****** filesysbox.library/--copyright-- ************************************
*
*   LIBRARY
*       Copyright (c) 2008-2011 Leif Salomonsson.
*       Copyright (c) 2013-2026 Fredrik Wikstrom.
*       This library is released under AROS PUBLIC LICENSE v.1.1.
*       See the file LICENSE.APL.
*
*   AUTODOC
*       Copyright (c) 2011 Leif Salomonsson.
*       Copyright (c) 2013-2026 Fredrik Wikstrom.
*       This material has been released under and is subject to
*       the terms of the Common Documentation License, v.1.0.
*       See the file LICENSE.CDL.
*
****************************************************************************/

#include "filesysbox_internal.h"
#include "fuse_stubs.h"
#include <devices/input.h>
#include <devices/inputevent.h>
#include <errno.h>
#include <string.h>

struct FileSysStartupMsg *FbxGetFSSM(struct Library *sysbase, struct DeviceNode *devnode) {
	struct Library *SysBase = sysbase;
	if (IS_VALID_BPTR(devnode->dn_Startup)) {
		struct FileSysStartupMsg *fssm = BADDR(devnode->dn_Startup);

		if (TypeOfMem(fssm)) {
			if (IS_VALID_BPTR(fssm->fssm_Device) && IS_VALID_BPTR(fssm->fssm_Environ)) {
				UBYTE           *device  = BADDR(fssm->fssm_Device);
				struct DosEnvec *environ = BADDR(fssm->fssm_Environ);

				if (TypeOfMem(device) && TypeOfMem(environ)) {
					return fssm;
				}
			}
		}
	}

	return NULL;
}

BOOL FbxCheckString(struct FbxFS *fs, const char *str) {
	const char *s = str;
	int c;

	CDEBUGF("FbxCheckString(%#p, '%s')\n", fs, str);

	while ((c = utf8_decode_slow(&s)) > 0);
	if (c != '\0') {
		CDEBUGF("Invalid UTF-8 sequence detected at character position: %d\n", (int)(s - str));
		return FALSE;
	}

	return TRUE;
}

size_t FbxStrlen(struct FbxFS *fs, const char *str) {
	return utf8_strlen(str);
}

char *FbxStrskip(struct FbxFS *fs, const char *str, size_t n) {
	return utf8_strskip(str, n);
}

int FbxStrcmp(struct FbxFS *fs, const char *s1, const char *s2) {
	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE) {
		return strcmp(s1, s2);
	} else {
		return utf8_stricmp(s1, s2);
	}
}

int FbxStrncmp(struct FbxFS *fs, const char *s1, const char *s2, size_t n) {
	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE) {
		return utf8_strncmp(s1, s2, n);
	} else {
		return utf8_strnicmp(s1, s2, n);
	}
}

size_t FbxStrlcpy(struct FbxFS *fs, char *dst, const char *src, size_t dst_size) {
	return utf8_strlcpy(dst, src, dst_size);
}

size_t FbxStrlcat(struct FbxFS *fs, char *dst, const char *src, size_t dst_size) {
	return utf8_strlcat(dst, src, dst_size);
}

static IPTR FbxHashPathInoCase(struct FbxFS *fs, const char *str) {
	IPTR v = 0;
	ULONG c;

	DEBUGF("FbxHashPathInoCase(%#p, '%s')\n", fs, str);

	// get a small seed from number of characters
	v = FbxStrlen(fs, str);

	// compute hash
	while ((c = utf8_decode_fast(&str)) != '\0') {
		v = v * 13 + c;
	}

	return v;
}

static IPTR FbxHashPathInoNoCase(struct FbxFS *fs, const char *str) {
	IPTR v = 0;
	ULONG c;

	DEBUGF("FbxHashPathInoNoCase(%#x, '%s')\n", fs, str);

	// get a small seed from number of characters
	v = FbxStrlen(fs, str);

	// compute hash
	while ((c = utf8_decode_fast(&str)) != '\0') {
		c = ucs4_toupper(c);
		v = v * 13 + c;
	}

	return v;
}

IPTR FbxHashPathIno(struct FbxFS *fs, const char *str) {
	DEBUGF("FbxHashPathIno(%#p, '%s')\n", fs, str);

	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE)
		return FbxHashPathInoCase(fs, str);
	else
		return FbxHashPathInoNoCase(fs, str);
}

/* Same as FbxHashPathIno() but with excess bits masked out */
static unsigned int FbxHashPath(struct FbxFS *fs, const char *str) {
	return FbxHashPathIno(fs, str) & ENTRYHASHMASK;
}

struct FbxEntry *FbxFindEntry(struct FbxFS *fs, const char *path) {
	unsigned int i;
	struct FbxEntry *e, *succ;

	DEBUGF("FbxFindEntry(%#p, '%s')\n", fs, path);

	i = FbxHashPath(fs, path);
	e = (struct FbxEntry *)fs->currvol->entrytab[i].mlh_Head;
	while ((succ = (struct FbxEntry *)e->hashchain.mln_Succ) != NULL) {
		if (FbxStrcmp(fs, e->path, path) == 0) return e;
		e = succ;
	}

	return NULL;
}

struct FbxLock *FbxLockEntry(struct FbxFS *fs, struct FbxEntry *e, int mode) {
	struct Library *SysBase = fs->sysbase;
	struct FbxLock *lock;

	DEBUGF("FbxLockEntry(%#p, %#p, %d)\n", fs, e, mode);

	// exclusive locks are not allowed on directories
	if (e->type == ETYPE_DIR && mode == EXCLUSIVE_LOCK) {
		mode = SHARED_LOCK;
	}

	if (mode == EXCLUSIVE_LOCK) {
		if (!IsMinListEmpty(&e->locklist)) {
			fs->r2 = ERROR_OBJECT_IN_USE;
			return NULL;
		}
	}

	if (e->xlock) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return NULL;
	}

	lock = AllocFbxLock();
	if (lock == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return NULL;
	}

	if (mode == EXCLUSIVE_LOCK)
		e->xlock = TRUE;

	lock->link       = ZERO;
	lock->diskid     = (IPTR)e->diskkey;
	lock->access     = mode;
	lock->taskmp     = fs->fsport;
	lock->volumebptr = MKBADDR(fs->currvol);
	lock->entry      = e;
	lock->info       = NULL;
	lock->dostype    = fs->dostype;
	lock->fsvol      = fs->currvol;
	lock->fs         = fs;
	lock->fh         = NULL;
	lock->mempool    = NULL;
	lock->dirscan    = FALSE;
	lock->filepos    = 0;
	lock->flags      = 0;

	NEWMINLIST(&lock->dirdatalist);

	AddTail((struct List *)&e->locklist, (struct Node *)&lock->entrychain);
	AddTail((struct List *)&lock->fsvol->locklist, (struct Node *)&lock->volumechain);

	DEBUGF("FbxLockEntry: lock %#p\n", lock);

	return lock;
}

void FbxEndLock(struct FbxFS *fs, struct FbxLock *lock) {
	struct Library *SysBase = fs->sysbase;

	DEBUGF("FbxEndLock(%#p, %#p)\n", fs, lock);

	Remove((struct Node *)&lock->entrychain);
	Remove((struct Node *)&lock->volumechain);

	if (lock->mempool != NULL) {
		DeletePool(lock->mempool);
	}

	lock->fs = NULL; // invalidate lock
	lock->info = NULL;

	if (lock->fsvol != fs->currvol &&
		IsMinListEmpty(&lock->fsvol->locklist))
	{
		Remove((struct Node *)&lock->fsvol->fschain);
		FreeFbxVolume(lock->fsvol);
	}

	FreeFbxLock(lock);
}

void FbxAddEntry(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	unsigned int i;

	DEBUGF("FbxAddEntry(%#p, %#p)\n", fs, e);

	i = FbxHashPath(fs, e->path);
	AddTail((struct List *)&fs->currvol->entrytab[i], (struct Node *)&e->hashchain);
}

static const char *FbxSkipColon(const char *s) {
	const char *s2 = strrchr(s, ':');
	return s2 ? (s2 + 1) : s;
}

BOOL FbxLockName2Path(struct FbxFS *fs, struct FbxLock *lock, const char *name, char *fullpathbuf) {
	char tname[FBX_MAX_NAME];
	char *p;
	size_t len;

	RDEBUGF("FbxLockName2Path(%#p, '%s', %#p)\n", lock, name, fullpathbuf);

	if (lock != NULL)
		FbxStrlcpy(fs, fullpathbuf, lock->entry->path, FBX_MAX_PATH);
	else
		FbxStrlcpy(fs, fullpathbuf, "/", FBX_MAX_PATH);

	name = FbxSkipColon(name);

	while (*name != '\0') {
		while (*name == '/') {
			if (!FbxParentPath(fs, fullpathbuf)) {
				fullpathbuf[0] = '\0';
				return FALSE;
			}
			name++;
		}
		if (*name != '\0') {
			p = strchr(name, '/');
			if (p == NULL) p = strchr(name, '\0');

			len = p - name;
			if (len >= sizeof(tname))
				return FALSE;

			FbxStrlcpy(fs, tname, name, len + 1);

			if (IsDotOrDotDot(tname))
				return FALSE;

			if (!IsRoot(fullpathbuf))
				FbxStrlcat(fs, fullpathbuf, "/", FBX_MAX_PATH);

			FbxStrlcat(fs, fullpathbuf, tname, FBX_MAX_PATH);
			name = p;
			if (*name != '\0') name++;
		}
	}

	RDEBUGF("FbxLockName2Path: DONE => '%s'\n", fullpathbuf);
	return TRUE;
}

int FbxFuseErrno2Error(int error) {
	switch (-error) {
	case ENOSYS:    /* Function not implemented */  return ERROR_ACTION_NOT_KNOWN;
	case EPERM:     /* Operation not permitted */   return -1;
	case ENOENT:    /* No such file or directory */ return ERROR_OBJECT_NOT_FOUND;
	case ESRCH:     /* No such process */           return -1;
	case EINTR:     /* Interrupted system call */   return -1;
	case EIO:       /* Input/output error */        return -1;
	case ENXIO:     /* Device not configured */     return -1;
	case E2BIG:     /* Argument list too long */    return ERROR_TOO_MANY_ARGS;
	case ENOEXEC:   /* Exec format error */         return -1;
	case EBADF:     /* Bad file descriptor */       return -1;
	case ECHILD:    /* No child processes */        return -1;
	case EDEADLK:   /* Resource deadlock avoided */ return -1;
	case ENOMEM:    /* Cannot allocate memory */    return ERROR_NO_FREE_STORE;
	case EACCES:    /* Permission denied */         return -1;
	case EFAULT:    /* Bad address */               return -1;
#ifdef ENOTBLK
	case ENOTBLK:   /* Block device required */     return ERROR_OBJECT_WRONG_TYPE;
#endif
	case EBUSY:     /* Device busy */               return -1;
	case EEXIST:    /* File exists */               return ERROR_OBJECT_EXISTS;
	case EXDEV:     /* Cross-device link */         return -1;
	case ENODEV:    /* Operation not supported by device */ return ERROR_NO_DISK;
	case ENOTDIR:   /* Not a directory */           return ERROR_OBJECT_WRONG_TYPE;
	case EISDIR:    /* Is a directory */            return ERROR_OBJECT_WRONG_TYPE;
	case EINVAL:    /* Invalid argument */          return ERROR_BAD_NUMBER;
	case ENFILE:    /* Too many open files in system */ return -1;
	case EMFILE:    /* Too many open files */       return -1;
	case ENOTTY:    /* Inappropriate ioctl for device */ return ERROR_OBJECT_WRONG_TYPE;
	case ETXTBSY:   /* Text file busy */            return ERROR_OBJECT_IN_USE;
	case EFBIG:     /* File too large */            return -1;
	case ENOSPC:    /* No space left on device */   return ERROR_DISK_FULL;
	case ESPIPE:    /* Illegal seek */              return ERROR_SEEK_ERROR;
	case EROFS:     /* Read-only file system */     return ERROR_WRITE_PROTECTED;
	case EMLINK:    /* Too many links */            return -1;
	case EPIPE:     /* Broken pipe */               return -1;
	case ENOTEMPTY: /* Directory not empty */       return ERROR_DIRECTORY_NOT_EMPTY;
	case EOPNOTSUPP: /* Operation not supported on socket */ return ERROR_ACTION_NOT_KNOWN;
	default:     
		debugf("FbxFuseErrno2Error: unknown fuse error %ld\n", error);
		return -1;
	}
}

void FbxSetEntryPath(struct FbxFS *fs, struct FbxEntry *e, const char *p) {
	FbxStrlcpy(fs, e->path, p, FBX_MAX_PATH);
}

struct FbxEntry *FbxSetupEntry(struct FbxFS *fs, const char *path, int type, QUAD id) {
	struct Library *SysBase = fs->sysbase;
	struct FbxEntry *e;

	DEBUGF("FbxSetupEntry(%#p, '%s', %d, 0x%llx)\n", fs, path, type, id);

	e = AllocFbxEntry(fs);
	if (e == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return NULL;
	}

	FbxSetEntryPath(fs, e, path);
	NEWMINLIST(&e->locklist);
	NEWMINLIST(&e->notifylist);
	e->xlock = FALSE;
	e->type = type;

	if (fs->fsflags & FBXF_USE_INO)
		e->diskkey = id;
	else
		e->diskkey = FbxHashPathIno(fs, path);

	FbxAddEntry(fs, e); // add to hash

	fs->r2 = 0;
	return e;
}

void FbxCleanupEntry(struct FbxFS *fs, struct FbxEntry *e) {
	DEBUGF("FbxCleanupEntry(%#p, %#p)\n", fs, e);
	if (e != NULL) {
		struct Library *SysBase = fs->sysbase;

		DEBUGF("FbxCleanupEntry: path '%s'\n", e->path);

		if (IsMinListEmpty(&e->notifylist) && IsMinListEmpty(&e->locklist)) {
			Remove((struct Node *)&e->hashchain);
			FbxStrlcpy(fs, e->path, "<<im free!>>", FBX_MAX_PATH);
			FreeFbxEntry(fs, e);
			DEBUGF("FbxCleanupEntry: freed entry %#p\n", e);
		}
	}
}

BOOL FbxCheckLock(struct FbxFS *fs, struct FbxLock *lock) {
	if (lock->fs != fs) {
		DEBUGF("INVALID LOCK %#p, -> cookie %#p (should be %#p)\n", lock, lock->fs, fs);
		return FALSE;
	}
	return TRUE;
}

void FbxSetModifyState(struct FbxFS *fs, int state) {
	if (state) {
		fs->lastmodify = FbxGetUpTimeMillis(fs);
		if (fs->lastmodify == 0) fs->lastmodify++; /* Don't set to zero */
		if (fs->firstmodify == 0)
			fs->firstmodify = fs->lastmodify;
	} else {
		fs->firstmodify = 0;
		fs->lastmodify = 0;
	}
}

BOOL FbxIsParent(struct FbxFS *fs, const char *parent, const char *child) {
	int plen = IsRoot(parent) ? 0 : FbxStrlen(fs, parent);
	if (FbxStrncmp(fs, parent, child, plen) == 0 && *FbxStrskip(fs, child, plen) == '/')
		return TRUE;
	else
		return FALSE;
}

void FbxTimeSpec2DS(struct FbxFS *fs, const struct timespec *ts, struct DateStamp *ds) {
	ULONG sec, nsec;

	sec = ts->tv_sec;
	nsec = ts->tv_nsec;

	// subtract 8 years of seconds to adjust for different epoch.
	sec -= UNIXTIMEOFFSET;

	/* convert to local time */
	sec -= fs->gmtoffset * 60;

	// check for overflow (if date was < 1.1.1978)
	if (sec > (ULONG)ts->tv_sec) {
		sec = 0;
		nsec = 0;
	}

	ds->ds_Days = sec / (60*60*24);
	ds->ds_Minute = (sec % (60*60*24)) / 60;
	ds->ds_Tick = (sec % 60) * TICKS_PER_SECOND + nsec / ((1000*1000*1000) / TICKS_PER_SECOND);
}

int FbxFlushAll(struct FbxFS *fs) {
	PDEBUGF("FbxFlushAll(%#p)\n", fs);

	if (OKVOLUME(fs->currvol)) {
		Fbx_fsync(fs, "", 0, NULL);
	}

	// reset update timeouts
	FbxSetModifyState(fs, 0);

	fs->r2 = 0;
	return DOSTRUE;
}

void FbxNotifyDiskChange(struct FbxFS *fs, UBYTE ieclass) {
	struct Library *SysBase = fs->sysbase;
	struct MsgPort *inputmp;
	struct IOStdReq *inputio;
	struct InputEvent ie;
	struct timeval tv;

	DEBUGF("FbxNotifyDiskChange(%#p, %#x)\n", fs, ieclass);

	inputmp = CreateMsgPort();
	inputio = CreateIORequest(inputmp, sizeof(struct IOStdReq));
	if (inputio != NULL) {
		if (OpenDevice((CONST_STRPTR)"input.device", 0, (struct IORequest *)inputio, 0) == 0) {
			struct Device *TimerBase = fs->timerbase;

			GetSysTime(&tv);

			ie.ie_NextEvent    = 0;
			ie.ie_Class        = ieclass;
			ie.ie_SubClass     = 0;
			ie.ie_Code         = 0;
			ie.ie_Qualifier    = IEQUALIFIER_MULTIBROADCAST;
			ie.ie_EventAddress = 0;

			//ie.ie_TimeStamp = tv;
			ie.ie_TimeStamp.tv_secs  = tv.tv_secs;
			ie.ie_TimeStamp.tv_micro = tv.tv_micro;

			inputio->io_Command = IND_WRITEEVENT;
			inputio->io_Length  = sizeof(struct InputEvent);
			inputio->io_Data    = &ie;

			DoIO((struct IORequest *)inputio);
			CloseDevice((struct IORequest *)inputio);
		}
		DeleteIORequest(inputio);
	}
	DeleteMsgPort(inputmp);
}

