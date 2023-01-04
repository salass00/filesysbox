/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
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
*       Copyright (c) 2013-2023 Fredrik Wikstrom.
*       This library is released under AROS PUBLIC LICENSE v.1.1.
*       See the file LICENSE.APL.
*
*   AUTODOC
*       Copyright (c) 2011 Leif Salomonsson.
*       Copyright (c) 2013-2023 Fredrik Wikstrom.
*       This material has been released under and is subject to
*       the terms of the Common Documentation License, v.1.0.
*       See the file LICENSE.CDL.
*
****************************************************************************/

#include "filesysbox_internal.h"
#include "fuse_stubs.h"
#include <devices/input.h>
#include <devices/inputevent.h>
#include <devices/trackdisk.h>
#include <errno.h>
#include <string.h>
#include <SDI/SDI_interrupt.h>

#define DOS_OWNER_ROOT 65535
#define DOS_OWNER_NONE 0

//#ifdef __AROS__
#define ts_sec  tv_sec
#define ts_nsec tv_nsec
//#endif

#ifdef __AROS__
#define ID_BUSY_DISK AROS_MAKE_ID('B','U','S','Y')
#else
#define ID_BUSY_DISK (0x42555359L)
#endif

void CopyStringBSTRToC(BSTR bstr, char *cstr, size_t size) {
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	strlcpy(cstr, (const char *)bstr, size);
#else
	UBYTE *src = BADDR(bstr);
	size_t len = *src++;
	if (len >= size) len = size-1;
	memcpy(cstr, src, len);
	cstr[len] = '\0';
#endif
}

void CopyStringCToBSTR(const char *cstr, BSTR bstr, size_t size) {
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	strlcpy((char *)bstr, cstr, size);
#else
	UBYTE *dst = BADDR(bstr);
	size_t len = strlen(cstr);
	if (len >= size) len = size-1;
	*dst++ = len;
	memcpy(dst, cstr, len);
#endif
}

#ifdef __AROS__
AROS_UFH5(int, FbxDiskChangeInterrupt,
	AROS_UFHA(APTR, data, A1),
	AROS_UFHA(APTR, code, A5),
	AROS_UFHA(struct ExecBase *, SysBase, A6),
	AROS_UFHA(APTR, mask, D1),
	AROS_UFHA(APTR, custom, A0))
{
	AROS_USERFUNC_INIT
#else
INTERRUPTPROTO(FbxDiskChangeInterrupt, int, APTR custom, APTR data) {
#endif
	struct FbxFS *fs = data;
	struct FbxDiskChangeHandler *dch = fs->diskchangehandler;

	if (dch != NULL) {
		dch->func(fs);
	}

	return 0;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

struct FbxDiskChangeHandler *FbxAddDiskChangeHandler(struct FbxFS *fs, FbxDiskChangeHandlerFunc func) {
	struct Library *SysBase = fs->sysbase;
	struct FbxDiskChangeHandler *dch;
	struct FileSysStartupMsg *fssm = fs->fssm;
	struct MsgPort *mp = NULL;
	struct IOStdReq *io = NULL;
	struct Interrupt *interrupt = NULL;
	char devname[256];

	dch = AllocPooled(fs->mempool, sizeof(*dch));
	if (dch == NULL) goto cleanup;

	dch->func = func;

	mp = CreateMsgPort();
	io = (struct IOStdReq *)CreateIORequest(mp, sizeof(*io));
	if (io == NULL) goto cleanup;

	CopyStringBSTRToC(fssm->fssm_Device, devname, sizeof(devname));
	if (OpenDevice((CONST_STRPTR)devname, fssm->fssm_Unit, (struct IORequest *)io, fssm->fssm_Flags) != 0) {
		io->io_Device = NULL;
		goto cleanup;
	}

	interrupt = AllocPooled(fs->mempool, sizeof(*interrupt));
	if (interrupt == NULL) goto cleanup;

	bzero(interrupt, sizeof(*interrupt));
	interrupt->is_Node.ln_Type = NT_INTERRUPT;
#ifdef __AROS__
	interrupt->is_Node.ln_Name = (char *)AROS_BSTR_ADDR(fs->devnode->dn_Name);
	interrupt->is_Code         = (void (*)())FbxDiskChangeInterrupt;
#else
	interrupt->is_Node.ln_Name = (char *)BADDR(fs->devnode->dn_Name) + 1;
	interrupt->is_Code         = (void (*)())ENTRY(FbxDiskChangeInterrupt);
#endif
	interrupt->is_Data = fs;

	io->io_Command = TD_ADDCHANGEINT;
	io->io_Data    = interrupt;
	io->io_Length  = sizeof(*interrupt);
	SendIO((struct IORequest *)io);

	dch->io = io;

	fs->diskchangehandler = dch;
	return dch;

cleanup:
	FreePooled(fs->mempool, interrupt, sizeof(*interrupt));
	if (io != NULL && io->io_Device != NULL) {
		CloseDevice((struct IORequest *)io);
	}
	DeleteIORequest((struct IORequest *)io);
	DeleteMsgPort(mp);
	FreePooled(fs->mempool, dch, sizeof(*dch));

	return NULL;
}

void FbxRemDiskChangeHandler(struct FbxFS *fs) {
	struct FbxDiskChangeHandler *dch = fs->diskchangehandler;

	if (dch != NULL) {
		struct Library *SysBase = fs->sysbase;
		struct IOStdReq *io = dch->io;
		struct MsgPort *mp = io->io_Message.mn_ReplyPort;
		struct Interrupt *interrupt = (struct Interrupt *)io->io_Data;

		if (CheckIO((struct IORequest *)io) == NULL) {
			io->io_Command = TD_REMCHANGEINT;
			DoIO((struct IORequest *)io);
		} else {
			WaitIO((struct IORequest *)io);
		}

		CloseDevice((struct IORequest *)io);
		DeleteIORequest((struct IORequest *)io);
		DeleteMsgPort(mp);
		FreePooled(fs->mempool, interrupt, sizeof(*interrupt));
		FreePooled(fs->mempool, dch, sizeof(*dch));
	}
}

BOOL FbxCheckString(struct FbxFS *fs, const char *str) {
	CDEBUGF("FbxCheckString(%#p, '%s')\n", fs, str);
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		const char *s = str;
		int c;
		while ((c = utf8_decode_slow(&s)) > 0);
		if (c != '\0') {
			CDEBUGF("Invalid UTF-8 sequence detected at character position: %d\n", (int)(s - str));
			return FALSE;
		}
	}
	return TRUE;
}

size_t FbxStrlen(struct FbxFS *fs, const char *str) {
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		return utf8_strlen(str);
	} else {
		return strlen(str);
	}
}

int FbxStrcmp(struct FbxFS *fs, const char *s1, const char *s2) {
	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE) {
		return strcmp(s1, s2);
	} else {
		if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
			return utf8_stricmp(s1, s2);
		} else {
			struct Library *UtilityBase = fs->utilitybase;
			return Stricmp((CONST_STRPTR)s1, (CONST_STRPTR)s2);
		}
	}
}

int FbxStrncmp(struct FbxFS *fs, const char *s1, const char *s2, size_t n) {
	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE) {
		return strncmp(s1, s2, n);
	} else {
		if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
			return utf8_strnicmp(s1, s2, n);
		} else {
			struct Library *UtilityBase = fs->utilitybase;
			return Strnicmp((CONST_STRPTR)s1, (CONST_STRPTR)s2, n);
		}
	}
}

size_t FbxStrlcpy(struct FbxFS *fs, char *dst, const char *src, size_t dst_size) {
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		return utf8_strlcpy(dst, src, dst_size);
	} else {
		return strlcpy(dst, src, dst_size);
	}
}

size_t FbxStrlcat(struct FbxFS *fs, char *dst, const char *src, size_t dst_size) {
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		return utf8_strlcat(dst, src, dst_size);
	} else {
		return strlcat(dst, src, dst_size);
	}
}

static unsigned int FbxHashPathCase(struct FbxFS *fs, const char *str) {
	unsigned int i, v = 0, c;

	DEBUGF("FbxHashPathCase(%#p, '%s')\n", fs, str);

	// get a small seed from number of characters
	v = FbxStrlen(fs, str);

	// compute hash
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		while ((c = utf8_decode_fast(&str)) != '\0') {
			v = v * 13 + c;
		}
	} else {
		while ((c = str[v]) != '\0') {
			v = v * 13 + c;
		}
	}

	// and mask away excess bits
	i = v & ENTRYHASHMASK;

	return i;
}

static unsigned int FbxHashPathNoCase(struct FbxFS *fs, const char *str) {
	unsigned int i, v = 0, c;

	DEBUGF("FbxHashPathNoCase(%#x, '%s')\n", fs, str);

	// get a small seed from number of characters
	v = FbxStrlen(fs, str);

	// compute hash
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		while ((c = utf8_decode_fast(&str)) != '\0') {
			c = ucs4_toupper(c);
			v = v * 13 + c;
		}
	} else {
		while ((c = str[v]) != '\0') {
			if (c >= 'a' && c <= 'z') c -= 32;
			if (c >= 224 && c <= 254 && c != 247) c -= 32;
			v = v * 13 + c;
		}
	}

	// and mask away excess bits
	i = v & ENTRYHASHMASK;

	return i;
}

static unsigned int FbxHashPath(struct FbxFS *fs, const char *str) {
	DEBUGF("FbxHashPath(%#p, '%s')\n", fs, str);

	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE)
		return FbxHashPathCase(fs, str);
	else
		return FbxHashPathNoCase(fs, str);
}

static struct FbxEntry *FbxFindEntry(struct FbxFS *fs, const char *path) {
	ULONG i;
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

static struct FbxLock *FbxLockEntry(struct FbxFS *fs, struct FbxEntry *e, int mode) {
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

	lock = AllocFbxLock(fs);
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
	lock->dirscan    = FALSE;
	lock->filepos    = 0;
	lock->flags      = 0;

	NEWLIST(&lock->dirdatalist);

	AddTail((struct List *)&e->locklist, (struct Node *)&lock->entrychain);
	AddTail((struct List *)&lock->fsvol->locklist, (struct Node *)&lock->volumechain);

	DEBUGF("FbxLockEntry: lock %#p\n", lock);

	return lock;
}

static void FreeFbxDirData(struct FbxFS *fs, struct FbxDirData *dd) {
#ifdef __AROS__
	struct Library *SysBase = fs->sysbase;
#endif

	DEBUGF("FreeFbxDirData(%#p, %#p)\n", fs, dd);

	if (dd != NULL) {
		if (dd->comment != NULL)
			FreeVecPooled(fs->mempool, dd->comment);

		FreeVecPooled(fs->mempool, dd);
	}
}

static void FreeFbxDirDataList(struct FbxFS *fs, struct MinList *list) {
	struct MinNode *chain, *succ;

	DEBUGF("FreeFbxDirDataList(%#p, %#p)\n", fs, list);

	chain = list->mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		FreeFbxDirData(fs, FSDIRDATAFROMNODE(chain));
		chain = succ;
	}

	NEWLIST(list);
}

static void FbxEndLock(struct FbxFS *fs, struct FbxLock *lock) {
	struct Library *SysBase = fs->sysbase;

	DEBUGF("FbxEndLock(%#p, %#p)\n", fs, lock);

	Remove((struct Node *)&lock->entrychain);
	Remove((struct Node *)&lock->volumechain);

	FreeFbxDirDataList(fs, &lock->dirdatalist);

	lock->fs = NULL; // invalidate lock
	lock->info = NULL;

	if (lock->fsvol != fs->currvol &&
		IsMinListEmpty(&lock->fsvol->locklist))
	{
		Remove((struct Node *)&lock->fsvol->fschain);
		FreeFbxVolume(lock->fsvol);
	}

	FreeFbxLock(fs, lock);
}

static void FbxAddEntry(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	ULONG i;

	DEBUGF("FbxAddEntry(%#p, %#p)\n", fs, e);

	i = FbxHashPath(fs, e->path);
	AddTail((struct List *)&fs->currvol->entrytab[i], (struct Node *)&e->hashchain);
}

static const char *FbxSkipColon(const char *s) {
	const char *s2 = strrchr(s, ':');
	return s2 ? (s2 + 1) : s;
}

static BOOL FbxParentPath(struct FbxFS *fs, char *pathbuf) {
	if (FbxStrcmp(fs, pathbuf, "/") == 0) {
		// can't parent root
		return FALSE;
	}
	char *p = strrchr(pathbuf, '/');
	if (p != NULL) {
		if (p == pathbuf) p++; // leave the root '/' alone
		*p = '\0';
		return TRUE;
	}
	// should never happen
	return FALSE;
}

static BOOL FbxLockName2Path(struct FbxFS *fs, struct FbxLock *lock, const char *name, char *fullpathbuf) {
	char tname[256];
	char *p;
	size_t len;

	RDEBUGF("FbxLockName2Path(%#p, '%s', %#p)\n", lock, name, fullpathbuf);

	if (lock != NULL)
		FbxStrlcpy(fs, fullpathbuf, lock->entry->path, MAXPATHLEN);
	else
		FbxStrlcpy(fs, fullpathbuf, "/", MAXPATHLEN);

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
			len = p - name + 1;
			if (len > sizeof(tname))
				return FALSE;
			FbxStrlcpy(fs, tname, name, len);
			if (FbxStrcmp(fs, fullpathbuf, "/") != 0 && tname[0])
				FbxStrlcat(fs, fullpathbuf, "/", MAXPATHLEN);
			FbxStrlcat(fs, fullpathbuf, tname, MAXPATHLEN);
			name = p;
			if (*name != '\0') name++;
		}
	}

	RDEBUGF("FbxLockName2Path: DONE => '%s'\n", fullpathbuf);
	return TRUE;
}

static int FbxFuseErrno2Error(int error) {
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

static void FbxDoNotifyRequest(struct FbxFS *fs, struct NotifyRequest *nr) {
	struct Library *SysBase = fs->sysbase;
	struct NotifyMessage *notifymsg;

	NDEBUGF("FbxDoNotifyRequest(%#p, %#p)\n", fs, nr);

	if (nr->nr_Flags & NRF_SEND_MESSAGE) {
		if ((nr->nr_MsgCount > 0) && (nr->nr_Flags & NRF_WAIT_REPLY)) {
			nr->nr_Flags |= NRF_MAGIC;
			NDEBUGF("DoNotifyRequest: did magic! nreq %#p '%s'\n", nr, nr->nr_FullName);
		} else {
			notifymsg = AllocNotifyMessage();
			if (!notifymsg) return; // lets abort.
			nr->nr_MsgCount++;
			notifymsg->nm_ExecMessage.mn_Length = sizeof(*notifymsg);
			notifymsg->nm_ExecMessage.mn_ReplyPort = fs->notifyreplyport;
			notifymsg->nm_Class = NOTIFY_CLASS;
			notifymsg->nm_Code = NOTIFY_CODE;
			notifymsg->nm_NReq = nr;
			PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *)notifymsg);
			NDEBUGF("DoNotifyRequest: sent message to port of nreq %#p '%s'\n", nr, nr->nr_FullName);
		}
	} else if (nr->nr_Flags & NRF_SEND_SIGNAL) {
		Signal(nr->nr_stuff.nr_Signal.nr_Task, 1 << nr->nr_stuff.nr_Signal.nr_SignalNum);
		NDEBUGF("DoNotifyRequest: signaled task of nreq %#p '%s'\n", nr, nr->nr_FullName);
	}
}

static void FbxDoNotifyEntry(struct FbxFS *fs, struct FbxEntry *entry) {
	struct MinNode *nnchain;
	struct NotifyRequest *nr;
	struct FbxNotifyNode *nn;

	NDEBUGF("FbxDoNotifyEntry(%#p, %#p)\n", fs, entry);

	nnchain = entry->notifylist.mlh_Head;
	while (nnchain->mln_Succ) {
		nn = FSNOTIFYNODEFROMCHAIN(nnchain);
		nr = nn->nr;
		FbxDoNotifyRequest(fs, nr);
		nnchain = nnchain->mln_Succ;
	}
}

static void FbxDoNotify(struct FbxFS *fs, const char *path) {
	struct FbxEntry *e;
	char *pathbuf = fs->pathbuf[2];

	NDEBUGF("FbxDoNotify(%#p, '%s')\n", fs, path);

	FbxStrlcpy(fs, pathbuf, path, MAXPATHLEN);
	do {
		e = FbxFindEntry(fs, pathbuf);
		if (e != NULL) FbxDoNotifyEntry(fs, e);
		// parent dirs wants notify too
	} while (FbxParentPath(fs, pathbuf));
}

static void FbxSetEntryPath(struct FbxFS *fs, struct FbxEntry *e, const char *p) {
	FbxStrlcpy(fs, e->path, p, MAXPATHLEN);
}

static struct FbxEntry *FbxSetupEntry(struct FbxFS *fs, const char *path, int type, QUAD id) {
	struct Library *SysBase = fs->sysbase;
	struct FbxEntry *e;

	DEBUGF("FbxSetupEntry(%#p, '%s', %d, 0x%llx)\n", fs, path, type, id);

	e = AllocFbxEntry(fs);
	if (e == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return NULL;
	}

	FbxSetEntryPath(fs, e, path);
	NEWLIST(&e->locklist);
	NEWLIST(&e->notifylist);
	e->xlock = FALSE;
	e->type = type;

	if (fs->fsflags & FBXF_USE_INO)
		e->diskkey = id;
	else
		e->diskkey = FbxHashPath(fs, path);

	FbxAddEntry(fs, e); // add to hash

	fs->r2 = 0;
	return e;
}

static void FbxCleanupEntry(struct FbxFS *fs, struct FbxEntry *e) {
	DEBUGF("FbxCleanupEntry(%#p, %#p)\n", fs, e);
	if (e != NULL) {
		struct Library *SysBase = fs->sysbase;

		DEBUGF("FbxCleanupEntry: path '%s'\n", e->path);

		if (IsMinListEmpty(&e->notifylist) && IsMinListEmpty(&e->locklist)) {
			Remove((struct Node *)&e->hashchain);
			FbxStrlcpy(fs, e->path, "<<im free!>>", MAXPATHLEN);
			FreeFbxEntry(fs, e);
			DEBUGF("FbxCleanupEntry: freed entry %#p\n", e);
		}
	}
}

static void FbxTryResolveNotify(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	struct FbxNotifyNode *nn;
	struct MinNode *chain, *succ;
	struct NotifyRequest *nr;
	char *fullpath = fs->pathbuf[2];

	NDEBUGF("FbxTryResolveNotify(%#p, %#p)\n", fs, e);

	chain = fs->currvol->unres_notifys.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		nn = FSNOTIFYNODEFROMCHAIN(chain);
		nr = nn->nr;
		if (FbxLockName2Path(fs, NULL, (const char *)nr->nr_FullName, fullpath) &&
			FbxStrcmp(fs, fullpath, e->path) == 0)
		{
			Remove((struct Node *)chain);
			AddTail((struct List *)&e->notifylist, (struct Node *)chain);
			nn->entry = e;
			PDEBUGF("try_resolve_nreqs: resolved nreq %#p, '%s'\n", nr, nr->nr_FullName);
		}
		chain = succ;
	}
}

static BOOL FbxCheckLock(struct FbxFS *fs, struct FbxLock *lock) {
	if (lock->fs != fs) {
		DEBUGF("INVALID LOCK %#p, -> cookie %#p (should be %#p)\n", lock, lock->fs, fs);
		return FALSE;
	}
	return TRUE;
}

static ULONG FbxGetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath) {
	char buffer[4];
	int res, i;
	ULONG prot = 0;

	res = Fbx_getxattr(fs, fullpath, fs->xattr_amiga_protection, buffer, sizeof(buffer));
	if (res == 4) {
		for (i = 0; i < 4; i++) {
			switch (buffer[i]) {
			case 'h': prot |= FIBF_HOLD; break;
			case 's': prot |= FIBF_SCRIPT; break;
			case 'p': prot |= FIBF_PURE; break;
			case 'a': prot |= FIBF_ARCHIVE; break;
			}
		}
	}

	return prot;
}

static int FbxSetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath, ULONG prot) {
	int error;

	if (prot & (FIBF_HOLD|FIBF_SCRIPT|FIBF_PURE|FIBF_ARCHIVE)) {
		char buffer[4];
		buffer[0] = (prot & FIBF_HOLD   ) ? 'h' : '-';
		buffer[1] = (prot & FIBF_SCRIPT ) ? 's' : '-';
		buffer[2] = (prot & FIBF_PURE   ) ? 'p' : '-';
		buffer[3] = (prot & FIBF_ARCHIVE) ? 'a' : '-';
		error = Fbx_setxattr(fs, fullpath, fs->xattr_amiga_protection, buffer, 4, 0);
	} else {
		error = Fbx_removexattr(fs, fullpath, fs->xattr_amiga_protection);
		if (error == -ENODATA)
			error = 0;
	}

	return error;
}

static void FbxClearArchiveFlags(struct FbxFS *fs, const char *fullpath) {
	char *pathbuf = fs->pathbuf[0];
	ULONG prot;

	FbxStrlcpy(fs, pathbuf, fullpath, MAXPATHLEN);
	do {
		prot = FbxGetAmigaProtectionFlags(fs, pathbuf);
		if (prot & FIBF_ARCHIVE) {
			prot &= ~FIBF_ARCHIVE;
			FbxSetAmigaProtectionFlags(fs, pathbuf, prot);
		}
	} while (FbxParentPath(fs, pathbuf) && FbxStrcmp(fs, pathbuf, "/") != 0);
}

static int FbxOpenLock(struct FbxFS *fs, struct FileHandle *fh, struct FbxLock *lock) {
	struct Library *SysBase = fs->sysbase;
	int error;

	PDEBUGF("FbxOpenLock(%#p, %#p, %#p)\n", fs, fh, lock);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (lock->entry->type != ETYPE_FILE) {
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	if (lock->info != NULL) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	lock->info = AllocFuseFileInfo(fs);
	if (lock->info == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return DOSFALSE;
	}

	if (fs->currvol->vflags & FBXVF_READ_ONLY)
		lock->info->flags = O_RDONLY;
	else
		lock->info->flags = O_RDWR;

	error = Fbx_open(fs, lock->entry->path, lock->info);
	if (error) {
		FreeFuseFileInfo(fs, lock->info);
		lock->info = NULL;
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	lock->fh = fh;
	fh->fh_Arg1 = (SIPTR)MKBADDR(lock);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxCreateOpenLock(struct FbxFS *fs, struct FileHandle *fh, struct FbxLock *lock, mode_t mode) {
	struct Library *SysBase = fs->sysbase;
	int error;

	PDEBUGF("FbxCreateOpenLock(%#p, %#p, %#p, 0%o)\n", fs, fh, lock, mode);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (lock->entry->type != ETYPE_FILE) {
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	if (lock->info != NULL) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	lock->info = AllocFuseFileInfo(fs);
	if (lock->info == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return DOSFALSE;
	}

	if (fs->currvol->vflags & FBXVF_READ_ONLY)
		lock->info->flags = O_RDONLY;
	else
		lock->info->flags = O_RDWR;

	error = Fbx_create(fs, lock->entry->path, mode, lock->info);
	if (error) {
		FreeFuseFileInfo(fs, lock->info);
		lock->info = NULL;
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	lock->fh = fh;
	fh->fh_Arg1 = (SIPTR)MKBADDR(lock);

	fs->r2 = 0;
	return DOSTRUE;
}

QUAD FbxGetUpTimeMillis(struct FbxFS *fs) {
	struct timeval tv;

	FbxGetUpTime(fs, &tv);
	return (UQUAD)tv.tv_secs * 1000 + tv.tv_micro / 1000;
}

void FbxSetModifyState(struct FbxFS *fs, int state) {
	if (state) {
		fs->lastmodify = FbxGetUpTimeMillis(fs);
		if (fs->firstmodify == 0)
			fs->firstmodify = fs->lastmodify;
	} else {
		fs->firstmodify = 0;
		fs->lastmodify = 0;
	}
}

static int FbxOpenFile(struct FbxFS *fs, struct FileHandle *fh,
	struct FbxLock *lock, const char *name, int mode)
{
	struct FbxEntry *e;
	int error, truncate = FALSE;
	int lockmode = SHARED_LOCK;
	struct fbx_stat statbuf;
	struct FbxLock *lock2 = NULL;
	char *fullpath = fs->pathbuf[0];
	int exists;

	DEBUGF("FbxOpenFile(%#p, %#p, %#p, '%s', %d)\n", fs, fh, lock, name, mode);

	CHECKVOLUME(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	e = FbxFindEntry(fs, fullpath);
	if (e != NULL) {
		if (e->type == ETYPE_DIR) {
			fs->r2 = ERROR_OBJECT_WRONG_TYPE;
			return DOSFALSE;
		}
		exists = TRUE;
	} else {
		error = Fbx_getattr(fs, fullpath, &statbuf);
		if (error == -ENOENT) {
			exists = FALSE;
		} else if (error == 0) {
			if (S_ISLNK(statbuf.st_mode)) {
				fs->r2 = ERROR_IS_SOFT_LINK;
				return DOSFALSE;
			} else if (!S_ISREG(statbuf.st_mode)) {
				fs->r2 = ERROR_OBJECT_WRONG_TYPE;
				return DOSFALSE;
			}
			exists = TRUE;
		} else {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	switch (mode) {
	case MODE_OLDFILE:
		if (!exists) {
			fs->r2 = ERROR_OBJECT_NOT_FOUND;
			return DOSFALSE;
		}
		break;
	case MODE_NEWFILE:
		lockmode = EXCLUSIVE_LOCK;
		if (e != NULL && !IsMinListEmpty(&e->locklist)) {
			/* locked before, a no go as we need exclusive */
			fs->r2 = ERROR_OBJECT_IN_USE;
			return DOSFALSE;
		}
		if (exists) {
			/* existed, lets truncate */
			truncate = TRUE;
			break;
		}
		/* fall through */
	case MODE_READWRITE:
		if (!exists) {
			/* did not exist, lets create */
			CHECKWRITABLE(DOSFALSE);
			if (FSOP mknod) {
				error = Fbx_mknod(fs, fullpath, DEFAULT_PERMS|S_IFREG, 0);
				if (error) {
					fs->r2 = FbxFuseErrno2Error(error);
					return DOSFALSE;
				}
				error = Fbx_getattr(fs, fullpath, &statbuf);
				if (error) {
					fs->r2 = FbxFuseErrno2Error(error);
					return DOSFALSE;
				}
			} else {
				e = FbxSetupEntry(fs, fullpath, ETYPE_FILE, 0);
				if (e == FALSE) return DOSFALSE;
				lock2 = FbxLockEntry(fs, e, lockmode);
				if (lock2 == NULL) {
					FbxCleanupEntry(fs, e);
					return DOSFALSE;
				}
				if (FbxCreateOpenLock(fs, fh, lock2, DEFAULT_PERMS) == DOSFALSE) {
					FbxEndLock(fs, lock2);
					FbxCleanupEntry(fs, e);
					return DOSFALSE;
				}
			}
			DEBUGF("FbxOpenFile: new file created ok\n");
		}
		break;
	default:
		fs->r2 = ERROR_BAD_NUMBER;
		return DOSFALSE;
	}

	if (truncate) {
		CHECKWRITABLE(DOSFALSE);
		error = Fbx_truncate(fs, fullpath, 0);
		if (error) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
		DEBUGF("FbxOpenFile: cleared ok\n");
	}

	if (e == NULL) {
		e = FbxSetupEntry(fs, fullpath, ETYPE_FILE, statbuf.st_ino);
		if (e == NULL) return DOSFALSE;
	}

	if (lock2 == NULL) {
		lock2 = FbxLockEntry(fs, e, lockmode);
		if (lock2 == NULL) {
			FbxCleanupEntry(fs, e);
			return DOSFALSE;
		}

		if (FbxOpenLock(fs, fh, lock2) == DOSFALSE) {
			FbxEndLock(fs, lock2);
			FbxCleanupEntry(fs, e);
			return DOSFALSE;
		}
	}

	if (!exists || truncate) {
		FbxTryResolveNotify(fs, e);
		lock2->flags |= LOCKFLAG_MODIFIED;
		FbxSetModifyState(fs, 1);
	}

	fs->r2 = 0;
	return DOSTRUE;
}

static void FbxUnResolveNotifys(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	struct MinNode *chain, *succ;
	struct FbxNotifyNode *nn;

	NDEBUGF("unresolve_notifys(%#p, %#p)\n", fs, e);

	// move possible notifys onto unresolved list
	chain = e->notifylist.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		Remove((struct Node *)chain);
		AddTail((struct List *)&fs->currvol->unres_notifys, (struct Node *)chain);
		nn = FSNOTIFYNODEFROMCHAIN(chain);
		NDEBUGF("unresolve_notifys: removed nn %#p\n", nn);
		nn->entry = NULL; // clear
		chain = succ;
	}

	NDEBUGF("unresolve_notifys: DONE\n");
}

static int FbxReadFile(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, int bytes) {
	int res;

	PDEBUGF("FbxReadFile(%#p, %#p, %#p, %d)\n", fs, lock, buffer, bytes);

	CHECKVOLUME(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	if (bytes == 0) {
		fs->r2 = 0;
		return 0;
	}

	res = Fbx_read(fs, lock->entry->path, buffer, bytes, lock->filepos, lock->info);
	if (res < 0) {
		fs->r2 = FbxFuseErrno2Error(res);
		return -1;
	}

	lock->filepos += res;
	fs->r2 = 0;
	return res;
}

static int FbxWriteFile(struct FbxFS *fs, struct FbxLock *lock, CONST_APTR buffer, int bytes) {
	int res;

	PDEBUGF("FbxWriteFile(%#p, %#p, %#p, %d)\n", fs, lock, buffer, bytes);

	CHECKVOLUME(-1);
	CHECKWRITABLE(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	if (bytes == 0) {
		fs->r2 = 0;
		return 0;
	}

	res = Fbx_write(fs, lock->entry->path, buffer, bytes, lock->filepos, lock->info);
	if (res < 0) {
		fs->r2 = FbxFuseErrno2Error(res);
		return -1;
	}
	if (res == bytes) {
		lock->filepos += bytes;
		fs->r2 = 0;
	}

	lock->flags |= LOCKFLAG_MODIFIED; // for notification
	FbxSetModifyState(fs, 1);

	return res;
}

static QUAD FbxSeekFile(struct FbxFS *fs, struct FbxLock *lock, QUAD pos, int mode) {
	QUAD newpos, oldpos;
	int error;
	struct fbx_stat statbuf;

	PDEBUGF("FbxSeekFile(%#p, %#p, %lld, %d)\n", fs, lock, pos, mode);

	CHECKVOLUME(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	if (lock->info->nonseekable) {
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		return -1;
	}

	oldpos = lock->filepos;

	error = Fbx_fgetattr(fs, lock->entry->path, &statbuf, lock->info);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	switch (mode) {
	case OFFSET_BEGINNING:
		newpos = pos;
		break;
	case OFFSET_CURRENT:
		newpos = oldpos + pos;
		break;
	case OFFSET_END:
		newpos = statbuf.st_size + pos;
		break;
	default:
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	if (newpos < 0 || newpos > statbuf.st_size) {
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	lock->filepos = newpos;
	return oldpos;
}

static int FbxUnLockObject(struct FbxFS *fs, struct FbxLock *lock) {
	struct FbxEntry *e;

	PDEBUGF("FbxUnLockObject(%#p, %#p)\n", fs, lock);

	if (lock == NULL) {
		debugf("FbxUnLockObject got a NULL lock, btw.\n");
		fs->r2 = 0;
		return DOSTRUE;
	}

	CHECKLOCK(lock, DOSFALSE);

	e = lock->entry;

	FbxEndLock(fs, lock);
	FbxCleanupEntry(fs, e);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxCloseFile(struct FbxFS *fs, struct FbxLock *lock) {
	struct Library *SysBase = fs->sysbase;
	struct FbxEntry *e;

	PDEBUGF("FbxCloseFile(%#p, %#p)\n", fs, lock);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	e = lock->entry;

	if (lock->fh == NULL) {
		// this lock should be ended with FbxUnLockObject()
		debugf("FbxCloseFile: lock %#p was never opened!\n", lock);
		fs->r2 = ERROR_INVALID_LOCK;
		return DOSFALSE;
	}

	if (lock->info != NULL) {
		Fbx_release(fs, e->path, lock->info);
		FreeFuseFileInfo(fs, lock->info);
		lock->info = NULL;
	}

	lock->fh = NULL;

	if (lock->fsvol == fs->currvol && (lock->flags & LOCKFLAG_MODIFIED)) {
		FbxClearArchiveFlags(fs, e->path);
		FbxDoNotify(fs, e->path);
		FbxSetModifyState(fs, 1);
	}

	FbxEndLock(fs, lock);
	FbxCleanupEntry(fs, e);

	fs->r2 = 0;
	return DOSTRUE;
}

static QUAD FbxTruncNewSize(struct FbxFS *fs, struct FbxLock *lock, QUAD newsize) {
	struct FbxLock *fslock;
	struct MinNode *chain, *succ;

	PDEBUGF("FbxTruncNewSize(%#p, %#p, %lld)\n", fs, lock, newsize);

	chain = lock->entry->locklist.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		fslock = FSLOCKFROMENTRYCHAIN(chain);
		if (fslock != lock) {
			if (fslock->filepos > newsize) {
				newsize = fslock->filepos;
			}
		}
		chain = succ;
	}

	return newsize;
}

static QUAD FbxSetFileSize(struct FbxFS *fs, struct FbxLock *lock, QUAD offs, int mode) {
	QUAD oldsize, newsize;
	int error;
	struct fbx_stat statbuf;

	PDEBUGF("FbxSetFileSize(%#p, %#p, %lld, %d)\n", fs, lock, offs, mode);

	CHECKVOLUME(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	error = Fbx_fgetattr(fs, lock->entry->path, &statbuf, lock->info);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	oldsize = statbuf.st_size;

	switch (mode) {
	case OFFSET_BEGINNING:
		newsize = offs;
		break;
	case OFFSET_CURRENT:
		newsize = lock->filepos + offs;
		break;
	case OFFSET_END:
		newsize = oldsize + offs;
		break;
	default:
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	if (newsize == oldsize) {
		fs->r2 = 0;
		return oldsize;
	}

	CHECKWRITABLE(-1);

	// check if there are other locks to this file and
	// if any of their positions are above newsize.
	if (FbxTruncNewSize(fs, lock, newsize) > newsize) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return -1;
	}

	// check if our lock's position points beyond new filesize
	// if so, truncate position to new filesize..
	if (lock->filepos > newsize) lock->filepos = newsize;

	error = Fbx_ftruncate(fs, lock->entry->path, newsize, lock->info);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	lock->flags |= LOCKFLAG_MODIFIED; // for notification
	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return newsize;
}

static struct FbxLock *FbxLocateObject(struct FbxFS *fs, struct FbxLock *lock, const char *name, int lockmode) {
	char *fullpath = fs->pathbuf[0];
	struct fbx_stat statbuf;
	int error;
	LONG ntype;
	struct FbxEntry *e;
	struct FbxLock *lock2;

	PDEBUGF("FbxLocateObject(%#p, %#p, '%s', %d)\n", fs, lock, name, lockmode);

	CHECKVOLUME(NULL);

	if (lock != NULL) {
		CHECKLOCK(lock, NULL);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return NULL;
		}
	}

	CHECKSTRING(name, NULL);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return NULL;
	}

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return NULL;
	}

	if (S_ISLNK(statbuf.st_mode)) {
		fs->r2 = ERROR_IS_SOFT_LINK;
		return NULL;
	} else if (S_ISREG(statbuf.st_mode)) {
		ntype = ETYPE_FILE;
	} else {
		ntype = ETYPE_DIR;
	}

	e = FbxFindEntry(fs, fullpath);
	if (e == NULL) {
		e = FbxSetupEntry(fs, fullpath, ntype, statbuf.st_ino);
		if (e == NULL) return NULL;
	}

	lock2 = FbxLockEntry(fs, e, lockmode);
	if (lock2 == NULL) {
		FbxCleanupEntry(fs, e);
		return NULL;
	}

	fs->r2 = 0;
	return lock2;
}

static struct FbxLock *FbxDupLock(struct FbxFS *fs, struct FbxLock *lock) {
	PDEBUGF("FbxDupLock(%#x)\n", lock);

	CHECKVOLUME(NULL);

	if (lock != NULL) {
		CHECKLOCK(lock, NULL);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return NULL;
		}

		lock = FbxLockEntry(fs, lock->entry, SHARED_LOCK);
		if (lock == NULL) return NULL;
	} else {
		lock = FbxLocateObject(fs, NULL, "", SHARED_LOCK);
		if (lock == NULL) return NULL;
	}

	fs->r2 = 0;
	return lock;
}

static struct FbxLock *FbxCreateDir(struct FbxFS *fs, struct FbxLock *lock, const char *name) {
	char *fullpath = fs->pathbuf[0];
	struct FbxEntry *e;
	int error;
	struct FbxLock *lock2;
	struct fbx_stat statbuf;

	PDEBUGF("FbxCreateDir(%#p, %#p, '%s')\n", fs, lock, name);

	CHECKVOLUME(NULL);
	CHECKWRITABLE(NULL);

	if (lock != NULL) {
		CHECKLOCK(lock, NULL);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return NULL;
		}
	}

	CHECKSTRING(name, NULL);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return NULL;
	}

	e = FbxFindEntry(fs, fullpath);
	if (e != NULL) {
		fs->r2 = ERROR_OBJECT_EXISTS;
		return NULL;
	}

	error = Fbx_mkdir(fs, fullpath, DEFAULT_PERMS|S_IFDIR);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return NULL;
	}

	DEBUGF("FbxCreateDir created dir ok\n");

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return NULL;
	}

	e = FbxSetupEntry(fs, fullpath, ETYPE_DIR, statbuf.st_ino);
	if (e == NULL) return NULL;

	FbxTryResolveNotify(fs, e);

	FbxDoNotify(fs, fullpath);

	lock2 = FbxLockEntry(fs, e, SHARED_LOCK);
	if (lock2 == NULL) {
		FbxCleanupEntry(fs, e);
		return NULL;
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return lock2;
}

static int FbxDeleteObject(struct FbxFS *fs, struct FbxLock *lock, const char *name) {
	char *fullpath = fs->pathbuf[0];
	struct FbxEntry *e;
	int error;
	struct fbx_stat statbuf;

	PDEBUGF("FbxDeleteObject(%#p, %#p, '%s')\n", fs, lock, name);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	if (FbxStrcmp(fs, fullpath, "/") == 0) {
		// can't delete root
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	e = FbxFindEntry(fs, fullpath);
	if (e != NULL && !IsMinListEmpty(&e->locklist)) {
		// we cannot delete if there are locks
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	if (S_ISDIR(statbuf.st_mode)) {
		error = Fbx_rmdir(fs, fullpath);
	} else {
		error = Fbx_unlink(fs, fullpath);
	}
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	if (e != NULL) {
		FbxUnResolveNotifys(fs, e);
		FbxCleanupEntry(fs, e);
	}

	while (FbxParentPath(fs, fullpath)) {
		e = FbxFindEntry(fs, fullpath);
		if (e != NULL) FbxDoNotifyEntry(fs, e);
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static void FbxUpdatePaths(struct FbxFS *fs, const char *oldpath, const char *newpath) {
	struct Library *SysBase = fs->sysbase;
	char *tstr = fs->pathbuf[3];
	struct MinNode *chain, *succ;

	// TODO: unresolve+tryresolve notify for affected entries..

	// lets rename all subentries. subentries can be
	// found by comparing common path. do this by traversing
	// global hashtable and compare entry->path
	int plen = FbxStrlen(fs, oldpath);
	int a;
	if (FbxStrcmp(fs, oldpath, "/") == 0) plen = 0;
	for (a = 0; a < ENTRYHASHSIZE; a++) {
		chain = fs->currvol->entrytab[a].mlh_Head;
		while ((succ = chain->mln_Succ) != NULL) {
			struct FbxEntry *e = FSENTRYFROMHASHCHAIN(chain);
			if (FbxStrncmp(fs, oldpath, e->path, plen) == 0 && e->path[plen] == '/') {
				// match! let's update path
				FbxStrlcpy(fs, tstr, newpath, MAXPATHLEN);
				FbxStrlcat(fs, tstr, e->path + plen, MAXPATHLEN);
				FbxSetEntryPath(fs, e, tstr);
				// and rehash it
				Remove((struct Node *)&e->hashchain);
				FbxAddEntry(fs, e);
			}
			chain = succ;
		}
	}
}

static BOOL FbxIsParent(struct FbxFS *fs, const char *parent, const char *child) {
	int plen = FbxStrlen(fs, parent);
	if (FbxStrcmp(fs, parent, "/") == 0) plen = 0;
	if (FbxStrncmp(fs, parent, child, plen) == 0 && child[plen] == '/')
		return TRUE;
	else
		return FALSE;
}

static int FbxRenameObject(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	struct FbxLock *lock2, const char *name2)
{
	struct Library *SysBase = fs->sysbase;
	char *fullpath = fs->pathbuf[0];
	char *fullpath2 = fs->pathbuf[1];
	struct FbxEntry *e, *e2;
	struct fbx_stat statbuf;
	int error;

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}
	if (lock2 != NULL) {
		CHECKLOCK(lock2, DOSFALSE);

		if (lock2->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);
	CHECKSTRING(name2, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath) ||
		!FbxLockName2Path(fs, lock2, name2, fullpath2))
	{
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	if (FbxStrcmp(fs, fullpath, "/") == 0) {
		// can't rename root
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	if (FbxIsParent(fs, fullpath, fullpath2)) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	/* Make sure that the object to be renamed exists */
	e = FbxFindEntry(fs, fullpath);
	if (e == NULL)
	{
		error = Fbx_getattr(fs, fullpath, &statbuf);
		if (error)
		{
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	/* Check if source and destination are the same object */
	if (FbxStrcmp(fs, fullpath, fullpath2) == 0)
	{
		if ((fs->currvol->vflags & FBXVF_CASE_SENSITIVE) || strcmp(fullpath, fullpath2) == 0)
		{
			/* Nothing to do here */
			fs->r2 = 0;
			return DOSTRUE;
		}
	}
	else
	{
		/* Check if destination already exists */
		e2 = FbxFindEntry(fs, fullpath2);
		if (e2 != NULL) {
			fs->r2 = ERROR_OBJECT_EXISTS;
			return DOSFALSE;
		}
		error = Fbx_getattr(fs, fullpath2, &statbuf);
		if (error == 0) {
			fs->r2 = ERROR_OBJECT_EXISTS;
			return DOSFALSE;
		} else if (error != -ENOENT) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	error = Fbx_rename(fs, fullpath, fullpath2);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxDoNotify(fs, fullpath);

	//e = FbxFindEntry(fs, fullpath); /* Already done in code above */
	if (e != NULL) {
		FbxUnResolveNotifys(fs, e);
		FbxSetEntryPath(fs, e, fullpath2);
		Remove((struct Node *)&e->hashchain);
		FbxAddEntry(fs, e);
		FbxTryResolveNotify(fs, e);
	}

	FbxDoNotify(fs, fullpath2);

	FbxUpdatePaths(fs, fullpath, fullpath2);

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static struct FbxLock *FbxLocateParent(struct FbxFS *fs, struct FbxLock *lock) {
	char *pname = fs->pathbuf[2];
	const char *name;

	PDEBUGF("FbxLocateParent(%#p, %#p)\n", fs, lock);

	CHECKVOLUME(NULL);

	if (lock == NULL) {
		fs->r2 = 0; // yes
		return NULL;
	}

	CHECKLOCK(lock, NULL);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return NULL;
	}

	FbxStrlcpy(fs, pname, lock->entry->path, MAXPATHLEN);
	if (!FbxParentPath(fs, pname)) {
		// can't parent root
		fs->r2 = 0; // yes
		return NULL;
	}

	name = pname;
	if (*name == '/') name++; // skip preceding slash character
	lock = FbxLocateObject(fs, NULL, name, SHARED_LOCK);
	if (lock == NULL) return NULL;

	fs->r2 = 0;
	return lock;
}

static void FbxDS2TimeSpec(struct FbxFS *fs, const struct DateStamp *ds, struct timespec *ts) {
	ULONG sec, nsec;

	sec = ds->ds_Days * (60*60*24);
	sec += ds->ds_Minute * 60;
	sec += ds->ds_Tick / 50;
	nsec = (ds->ds_Tick % 50) * (1000*1000*1000/50);

	// add 8 years of seconds to adjust for different epoch.
	sec += UNIXTIMEOFFSET;

	/* convert to GMT */
	sec += fs->gmtoffset * 60;

	ts->ts_sec = sec;
	ts->ts_nsec = nsec;
}

void FbxTimeSpec2DS(struct FbxFS *fs, const struct timespec *ts, struct DateStamp *ds) {
	ULONG sec, nsec;

	sec = ts->ts_sec;
	nsec = ts->ts_nsec;

	// subtract 8 years of seconds to adjust for different epoch.
	sec -= UNIXTIMEOFFSET;

	/* convert to local time */
	sec -= fs->gmtoffset * 60;

	// check for overflow (if date was < 1.1.1978)
	if (sec > (ULONG)ts->ts_sec) {
		sec = 0;
		nsec = 0;
	}

	ds->ds_Days = sec / (60*60*24);
	ds->ds_Minute = (sec % (60*60*24)) / 60;
	ds->ds_Tick = (sec % 60) * 50 + nsec / (1000*1000*1000/50);
}

static int FbxSetDate(struct FbxFS *fs, struct FbxLock *lock, const char *name, const struct DateStamp *date) {
	struct timespec tv[2];
	int error;
	char *fullpath = fs->pathbuf[0];

	PDEBUGF("FbxSetDate(%#p, %#p, '%s', %#p)\n", fs, lock, name, date);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	FbxDS2TimeSpec(fs, date, &tv[0]);
	FbxDS2TimeSpec(fs, date, &tv[1]);

	if (FSOP utimens) {
		error = Fbx_utimens(fs, fullpath, tv);
	} else {
		struct utimbuf ubuf;

		ubuf.actime  = tv[0].ts_sec;
		ubuf.modtime = tv[1].ts_sec;

		error = Fbx_utime(fs, fullpath, &ubuf);
	}
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxDoNotify(fs, fullpath);

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static mode_t FbxProtection2Mode(ULONG prot) {
	mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR;

	if (prot & FIBF_READ       ) mode &= ~(S_IRUSR);
	if (prot & FIBF_WRITE      ) mode &= ~(S_IWUSR);
	if (prot & FIBF_EXECUTE    ) mode &= ~(S_IXUSR);
	if (prot & FIBF_GRP_READ   ) mode |= S_IRGRP;
	if (prot & FIBF_GRP_WRITE  ) mode |= S_IWGRP;
	if (prot & FIBF_GRP_EXECUTE) mode |= S_IXGRP;
	if (prot & FIBF_OTR_READ   ) mode |= S_IROTH;
	if (prot & FIBF_OTR_WRITE  ) mode |= S_IWOTH;
	if (prot & FIBF_OTR_EXECUTE) mode |= S_IXOTH;

	return mode;
}

static int FbxSetProtection(struct FbxFS *fs, struct FbxLock *lock, const char *name, ULONG prot) {
	int error;
	char *fullpath = fs->pathbuf[0];

	PDEBUGF("FbxSetProtection(%#p, %#p, '%s', %#lx)\n", fs, lock, name, prot);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_chmod(fs, fullpath, FbxProtection2Mode(prot));
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	error = FbxSetAmigaProtectionFlags(fs, fullpath, prot);
	if (error && (error != -ENOSYS && error != -EOPNOTSUPP)) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxSetComment(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const char *comment)
{
	char *fullpath = fs->pathbuf[0];
	int error;

	PDEBUGF("FbxSetComment(%#p, %#p, '%s', '%s')\n", fs, lock, name, comment);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);
	CHECKSTRING(comment, DOSFALSE);

	FbxLockName2Path(fs, lock, name, fullpath);

	if (comment[0] != '\0') {
		error = Fbx_setxattr(fs, fullpath, fs->xattr_amiga_comment,
			comment, strlen(comment), 0);
	} else {
		error = Fbx_removexattr(fs, fullpath, fs->xattr_amiga_comment);
		if (error == -ENODATA)
			error = 0;
	}
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static uid_t FbxAmiga2UnixOwner(const UWORD owner) {
	if (owner == DOS_OWNER_ROOT) return (uid_t)0;
	else if (owner == DOS_OWNER_NONE) return (uid_t)-2;
	else return (uid_t)owner;
}

static int FbxSetOwnerInfo(struct FbxFS *fs, struct FbxLock *lock, const char *name, UWORD uid, UWORD gid) {
	int error;
	char *fullpath = fs->pathbuf[0];

	PDEBUGF("FbxSetOwnerInfo(%#p, %#p, '%s', %#x, %#x)\n", fs, lock, name, uid, gid);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_chown(fs, fullpath, FbxAmiga2UnixOwner(uid), FbxAmiga2UnixOwner(gid));
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxSameLock(struct FbxFS *fs, struct FbxLock *lock, struct FbxLock *lock2) {
	PDEBUGF("FbxSameLock(%#p, %#p, %#p)\n", fs, lock, lock2);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);
	CHECKLOCK(lock2, DOSFALSE);

	fs->r2 = 0;
	if (lock == lock2) return DOSTRUE;
	if (lock->fsvol == lock2->fsvol) {
		struct FbxEntry *entry = lock->entry;
		struct FbxEntry *entry2 = lock2->entry;

		/* Check for same entries */
		if (entry == entry2)
			return DOSTRUE;

		/* Compare inodes (if valid) */
		if (fs->fsflags & FBXF_USE_INO)
		{
			if (entry->diskkey != 0 && entry->diskkey == entry2->diskkey)
				return DOSTRUE;
		}
	}
	return DOSFALSE;
}

static int FbxMakeHardLink(struct FbxFS *fs, struct FbxLock *lock, const char *name, struct FbxLock *lock2) {
	char *fullpath = fs->pathbuf[0];
	char *fullpath2 = fs->pathbuf[1];
	int error;

	PDEBUGF("FbxMakeHardlink(%#p, %#p, '%s', %#p)\n", fs, lock, name, lock2);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKLOCK(lock2, DOSFALSE);

	CHECKSTRING(name, DOSFALSE);

	if (lock2->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (!FbxLockName2Path(fs, lock, name, fullpath) ||
		!FbxLockName2Path(fs, lock2, "", fullpath2))
	{
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	if (FbxIsParent(fs, fullpath2, fullpath)) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	error = Fbx_link(fs, fullpath2, fullpath);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxDoNotify(fs, fullpath);

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxMakeSoftLink(struct FbxFS *fs, struct FbxLock *lock, const char *name, const char *softname) {
	char *fullpath = fs->pathbuf[0];
	int error;

	PDEBUGF("FbxMakeSoftlink(%#p, %#p, '%s', '%s')\n", fs, lock, name, softname);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);
	CHECKSTRING(softname, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_symlink(fs, softname, fullpath);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxDoNotify(fs, fullpath);

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxReadLink(struct FbxFS *fs, struct FbxLock *lock, const char *name, char *buffer, int len) {
	char *fullpath = fs->pathbuf[0];
	char *softname = fs->pathbuf[1];
	struct fbx_stat statbuf;
	int error;

	PDEBUGF("FbxReadLink(%#p, %#p, '%s', %#p, %d)\n", fs, lock, name, buffer, len);

	CHECKVOLUME(-1);

	if (lock != NULL) {
		CHECKLOCK(lock, -1);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return -1;
		}
	}

	CHECKSTRING(name, -1);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return -1;
	}

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	// check if it's a soft link
	if (!S_ISLNK(statbuf.st_mode)) {
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return -1;
	}

	error = Fbx_readlink(fs, fullpath, softname, MAXPATHLEN);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	if (FbxStrlcpy(fs, buffer, softname, len) >= len) {
		fs->r2 = ERROR_LINE_TOO_LONG;
		return -2;
	}

	fs->r2 = 0;
	return strlen(softname);
}

static int FbxChangeMode(struct FbxFS *fs, struct FbxLock *lock, int mode) {
	PDEBUGF("FbxChangeMode(%#p, %#p, %d)\n", fs, lock, mode);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (!OneInMinList(&lock->entry->locklist)) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	lock->access = mode;
	if (mode == EXCLUSIVE_LOCK) {
		lock->entry->xlock = TRUE;
	} else if (mode == SHARED_LOCK) {
		lock->entry->xlock = FALSE;
	} else {
		fs->r2 = ERROR_BAD_NUMBER;
		return DOSFALSE;
	}

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxExamineAllEnd(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, int len,
	int type, struct ExAllControl *ctrl)
{
	struct Library *SysBase = fs->sysbase;
	struct FbxExAllState *exallstate;

	PDEBUGF("FbxExamineAllEnd(%#p, %#p, %#p, %d, %d, %#p)\n", fs, lock, buffer, len, type, ctrl);

	if (ctrl != NULL) {
		exallstate = (struct FbxExAllState *)ctrl->eac_LastKey;
		if (exallstate) {
			if (exallstate != (APTR)-1) {
				FreeFbxDirDataList(fs, &exallstate->freelist);
				FreeFbxExAllState(fs, exallstate);
			}
			ctrl->eac_LastKey = (IPTR)NULL;
		}
	}

	if (lock != NULL) FreeFbxDirDataList(fs, &lock->dirdatalist);

	fs->r2 = 0;
	return DOSTRUE;
}

static int dir_fill_func(void *udata, const char *name, const struct fbx_stat *stat, fbx_off_t offset) {
	struct FbxLock *lock = udata;
	struct FbxFS *fs = lock->fs;
	struct Library *SysBase = fs->sysbase;
	struct FbxDirData *ed;
	size_t len;

	if (name == NULL) return 2;

	if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
		if (FbxCheckString(fs, name)) {
			len = strlen(name) + 1;
			ed = AllocFbxDirData(fs, len);
			if (ed == NULL) return 1;

			ed->name = (char *)(ed + 1);
			ed->comment = NULL;
			FbxStrlcpy(fs, ed->name, name, len);
			if (stat != NULL)
			{
				ed->stat = *stat;
			}
			else
			{
				ed->stat.st_ino = 0;
			}

			AddTail((struct List *)&lock->dirdatalist, (struct Node *)ed);
		}
	}

	return 0;
}

static LONG FbxMode2EntryType(const mode_t mode) {
	if (S_ISDIR(mode)) {
		return ST_USERDIR;
	} else if (S_ISREG(mode)) {
		return ST_FILE;
	} else if (S_ISLNK(mode)) {
		return ST_SOFTLINK;
	} else { // whatever..
		return ST_PIPEFILE;
	}
}

static ULONG FbxMode2Protection(const mode_t mode) {
	ULONG prot = FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE;

	if (mode & S_IRUSR) prot &= ~(FIBF_READ);
	if (mode & S_IWUSR) prot &= ~(FIBF_WRITE|FIBF_DELETE);
	if (mode & S_IXUSR) prot &= ~(FIBF_EXECUTE);
	if (mode & S_IRGRP) prot |= FIBF_GRP_READ;
	if (mode & S_IWGRP) prot |= FIBF_GRP_WRITE|FIBF_GRP_DELETE;
	if (mode & S_IXGRP) prot |= FIBF_GRP_EXECUTE;
	if (mode & S_IROTH) prot |= FIBF_OTR_READ;
	if (mode & S_IWOTH) prot |= FIBF_OTR_WRITE|FIBF_OTR_DELETE;
	if (mode & S_IXOTH) prot |= FIBF_OTR_EXECUTE;

	return prot;
}

static void FbxGetComment(struct FbxFS *fs, const char *fullpath, char *comment, size_t size) {
	int res;

	res = Fbx_getxattr(fs, fullpath, fs->xattr_amiga_comment, comment, size-1);
	if (res < 0) {
		comment[0] = '\0';
		return;
	}

	comment[min(res, size-1)] = '\0';
	if (!FbxCheckString(fs, comment)) {
		comment[0] = '\0';
		return;
	}
}

static UWORD FbxUnix2AmigaOwner(const uid_t owner) {
	if (owner == (uid_t)0) return DOS_OWNER_ROOT;
	else if (owner == (uid_t)-2) return DOS_OWNER_NONE;
	else return (UWORD)owner;
}

static int FbxReadDir(struct FbxFS *fs, struct FbxLock *lock) {
	int error;

	if (FSOP opendir != FSOP open) {
		struct Library *SysBase = fs->sysbase;
		struct fuse_file_info *fi;

		fi = AllocFuseFileInfo(fs);
		if (fi == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}

		error = Fbx_opendir(fs, lock->entry->path, fi);
		if (error) {
			FreeFuseFileInfo(fs, fi);
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}

		error = Fbx_readdir(fs, lock->entry->path, lock, dir_fill_func, 0, fi);
		if (error) {
			Fbx_releasedir(fs, lock->entry->path, fi);
			FreeFuseFileInfo(fs, fi);
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}

		Fbx_releasedir(fs, lock->entry->path, fi);
		FreeFuseFileInfo(fs, fi);
	} else {
		error = Fbx_readdir(fs, lock->entry->path, lock, dir_fill_func, 0, NULL);
		if (error) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	return DOSTRUE;
}

static int FbxExamineAll(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, SIPTR len,
	int type, struct ExAllControl *ctrl)
{
	struct Library *SysBase     = fs->sysbase;
	struct Library *DOSBase     = fs->dosbase;
	struct Library *UtilityBase = fs->utilitybase;
	struct FbxDirData *ed = NULL;
	int error, iptrs;
	struct FbxExAllState *exallstate;
	IPTR *lptr, *start, *prev;
	struct DateStamp ds;
	struct fbx_stat statbuf;
	char *fullpath = fs->pathbuf[0];
	char *comment = fs->pathbuf[1];

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	ctrl->eac_Entries = 0;

	if (ctrl->eac_LastKey == (IPTR)NULL) {
		exallstate = AllocFbxExAllState(fs);
		if (exallstate == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}

		FreeFbxDirDataList(fs, &lock->dirdatalist);
		NEWLIST(&exallstate->freelist);

		// read in entries
		if (!FbxReadDir(fs, lock)) {
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			FreeFbxExAllState(fs, exallstate);
			return DOSFALSE;
		}

		switch (type) {
		case ED_NAME:
			iptrs = 2;
			break;
		case ED_TYPE:
			iptrs = 3;
			break;
		case ED_SIZE:
			iptrs = 4;
			break;
		case ED_PROTECTION:
			iptrs = 5;
			break;
		case ED_DATE:
			iptrs = 8;
			break;
		case ED_COMMENT:
			iptrs = 9;
			break;
		case ED_OWNER:
			iptrs = 10;
			break;
		default:
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			FreeFbxExAllState(fs, exallstate);
			fs->r2 = ERROR_BAD_NUMBER; // unsupported ED_XXX
			return DOSFALSE;
		}

		exallstate->iptrs = iptrs;
		ctrl->eac_LastKey = (IPTR)exallstate;
	} else if (ctrl->eac_LastKey == (IPTR)-1) {
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	} else {
		exallstate = (struct FbxExAllState *)ctrl->eac_LastKey;
		// free previous exdata
		FreeFbxDirDataList(fs, &exallstate->freelist);
	}

	lptr = buffer;
	start = prev = NULL;
	*lptr = 0; // clear next pointer
	iptrs = exallstate->iptrs;
	while ((char *)&lptr[iptrs] <= ((char *)buffer + len)) {
		ed = (struct FbxDirData *)RemHead((struct List *)&lock->dirdatalist);
		if (ed == NULL) break;

		start = lptr;
		*lptr = (IPTR)NULL; // clear next pointer

		if (ctrl->eac_MatchString != NULL &&
			ctrl->eac_MatchFunc == NULL &&
			!MatchPatternNoCase(ctrl->eac_MatchString, (STRPTR)ed->name))
		{
			FreeFbxDirData(fs, ed);
			continue;
		}

		if (type >= ED_TYPE) {
			if (!FbxLockName2Path(fs, lock, ed->name, fullpath)) {
				FreeFbxDirData(fs, ed);
				fs->r2 = ERROR_INVALID_COMPONENT_NAME;
				return DOSFALSE;
			}

			if (fs->fsflags & FBXF_USE_FILL_DIR_STAT) {
				statbuf = ed->stat;
			} else {
				error = Fbx_getattr(fs, fullpath, &statbuf);
				if (error) {
					FreeFbxDirData(fs, ed);
					fs->r2 = FbxFuseErrno2Error(error);
					return DOSFALSE;
				}
			}
		}

		if (type >= ED_COMMENT) {
			FbxGetComment(fs, fullpath, comment, MAXPATHLEN);

			if (comment[0] != '\0') {
				size_t len = strlen(comment) + 1;

				ed->comment = AllocVecPooled(fs->mempool, len);
				if (ed->comment == NULL) {
					FreeFbxDirData(fs, ed);
					fs->r2 = ERROR_NO_FREE_STORE;
					return DOSFALSE;
				}

				FbxStrlcpy(fs, ed->comment, comment, len);
			}
		}

		lptr++; // skip next pointer

		if (type >= ED_NAME) *lptr++ = (IPTR)ed->name;
		if (type >= ED_TYPE) *lptr++ = FbxMode2EntryType(statbuf.st_mode);
		if (type >= ED_SIZE) *lptr++ = (statbuf.st_size > 0xffffffff) ? 0 : statbuf.st_size;
		if (type >= ED_PROTECTION) {
			*lptr = FbxMode2Protection(statbuf.st_mode);
			*lptr++ |= FbxGetAmigaProtectionFlags(fs, fullpath);
		}
		if (type >= ED_DATE) {
			FbxTimeSpec2DS(fs, &statbuf.st_mtim, &ds);
			*lptr++ = ds.ds_Days;
			*lptr++ = ds.ds_Minute;
			*lptr++ = ds.ds_Tick;
		}
		if (type >= ED_COMMENT) *lptr++ = (ed->comment != NULL) ? (IPTR)ed->comment : (IPTR)"";
		if (type >= ED_OWNER) {
			ULONG uid = FbxUnix2AmigaOwner(statbuf.st_uid);
			ULONG gid = FbxUnix2AmigaOwner(statbuf.st_gid);
			*lptr++ = (uid << 16)|gid;
		}

		if (ctrl->eac_MatchFunc != NULL &&
			!CallHookPkt(ctrl->eac_MatchFunc, &type, start))
		{
			FreeFbxDirData(fs, ed);
			lptr = start;
			continue;
		}

		AddTail((struct List *)&exallstate->freelist, (struct Node *)ed);

		ctrl->eac_Entries++;

		if (prev != NULL) *prev = (IPTR)start; // link us in
		prev = start;
	}

	if (IsMinListEmpty(&lock->dirdatalist)) {
		if (ctrl->eac_Entries == 0) {
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			FreeFbxExAllState(fs, exallstate);
			ctrl->eac_LastKey = (IPTR)-1;
		}
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	} else {
		fs->r2 = 0;
		return DOSTRUE;
	}
}

static const char *FbxFilePart(const char *path) {
	const char *name = strrchr(path, '/');
	return name ? (name + 1) : path;
}

static void FbxPathStat2FIB(struct FbxFS *fs, const char *fullpath,
	struct fbx_stat *stat, struct FileInfoBlock *fib)
{
	char *comment = fs->pathbuf[2];
	if (FbxStrcmp(fs, fullpath, "/") == 0) {
		FbxStrlcpy(fs, (char *)fib->fib_FileName + 1, fs->currvol->volname, sizeof(fib->fib_FileName));
		fib->fib_DirEntryType =
		fib->fib_EntryType = ST_ROOT;
	} else {
		FbxStrlcpy(fs, (char *)fib->fib_FileName + 1, FbxFilePart(fullpath), sizeof(fib->fib_FileName));
		fib->fib_DirEntryType =
		fib->fib_EntryType = FbxMode2EntryType(stat->st_mode);
	}
	fib->fib_FileName[0] = strlen((char *)fib->fib_FileName + 1);
	FbxGetComment(fs, fullpath, comment, MAXPATHLEN);
	FbxStrlcpy(fs, (char *)fib->fib_Comment + 1, comment, sizeof(fib->fib_Comment));
	fib->fib_Comment[0] = strlen((char *)fib->fib_Comment + 1);
	fib->fib_Size = stat->st_size;
	fib->fib_Protection = FbxMode2Protection(stat->st_mode);
	fib->fib_Protection |= FbxGetAmigaProtectionFlags(fs, fullpath);
	fib->fib_NumBlocks = stat->st_blocks;
	if (fs->fsflags & FBXF_USE_INO)
		fib->fib_DiskKey = (IPTR)stat->st_ino;
	else
		fib->fib_DiskKey = (IPTR)FbxHashPath(fs, fullpath);
	FbxTimeSpec2DS(fs, &stat->st_mtim, &fib->fib_Date);
	fib->fib_OwnerUID = FbxUnix2AmigaOwner(stat->st_uid);
	fib->fib_OwnerGID = FbxUnix2AmigaOwner(stat->st_gid);
}

static int FbxExamineObject(struct FbxFS *fs, struct FbxLock *lock, struct FileInfoBlock *fib) {
	struct fbx_stat statbuf;
	int error;

	PDEBUGF("FbxExamineObject(%#p, %#p, %#p)\n", fs, lock, fib);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);	

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	FreeFbxDirDataList(fs, &lock->dirdatalist);

	if (!lock->info) {
		error = Fbx_getattr(fs, lock->entry->path, &statbuf);
	} else {
		error = Fbx_fgetattr(fs, lock->entry->path, &statbuf, lock->info);
	}
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}
	FbxPathStat2FIB(fs, lock->entry->path, &statbuf, fib);
	lock->dirscan = FALSE;
	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxExamineNext(struct FbxFS *fs, struct FbxLock *lock, struct FileInfoBlock *fib) {
	struct Library *SysBase = fs->sysbase;
	struct FbxDirData *ed;
	struct fbx_stat statbuf;
	int error;
	char *fullpath = fs->pathbuf[0];

	PDEBUGF("FbxExamineNext(%#p, %#p, %#p)\n", fs, lock, fib);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (!lock->dirscan) {
		if (!FbxReadDir(fs, lock)) {
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			return DOSFALSE;
		}
		lock->dirscan = TRUE;
	}

	ed = (struct FbxDirData *)RemHead((struct List *)&lock->dirdatalist);
	if (ed == NULL) {
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	}

	if (!FbxLockName2Path(fs, lock, ed->name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	if (fs->fsflags & FBXF_USE_FILL_DIR_STAT) {
		statbuf = ed->stat;
	} else {
		error = Fbx_getattr(fs, fullpath, &statbuf);
		if (error) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	FbxPathStat2FIB(fs, ed->name, &statbuf, fib);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxAddNotify(struct FbxFS *fs, struct NotifyRequest *notify) {
	struct Library *SysBase = fs->sysbase;
	struct fbx_stat statbuf;
	struct FbxEntry *e;
	struct FbxNotifyNode *nn;
	char *fullpath = fs->pathbuf[0];
	LONG etype;
	int error;

	PDEBUGF("FbxAddNotify(%#p, %#p)\n", fs, notify);

	CHECKVOLUME(DOSFALSE);

	notify->nr_notifynode = (IPTR)NULL;
	notify->nr_MsgCount = 0;

	if (!FbxLockName2Path(fs, NULL, (char *)notify->nr_FullName, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		if (fs->r2 == -ENOENT) { // file did not exist
			NDEBUGF("FbxAddNotify: file '%s' did not exist.\n", fullpath);

			nn = AllocFbxNotifyNode();
			if (nn == NULL) {
				fs->r2 = ERROR_NO_FREE_STORE;
				return DOSFALSE;
			}

			nn->nr = notify;
			nn->entry = NULL;

			notify->nr_notifynode = (IPTR)nn;
			// lets put request on the unresolved list
			AddTail((struct List *)&fs->currvol->unres_notifys, (struct Node *)&nn->chain);
		} else {
			NDEBUGF("FbxAddNotify: getattr() error %d.\n", error);
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	} else { // file existed
		NDEBUGF("FbxAddNotify: file '%s' existed.\n", fullpath);

		if (S_ISREG(statbuf.st_mode)) {
			etype = ETYPE_FILE;
		} else {
			etype = ETYPE_DIR;
		}

		e = FbxFindEntry(fs, fullpath);
		if (e == NULL) {
			e = FbxSetupEntry(fs, fullpath, etype, statbuf.st_ino);
			if (e == NULL) return DOSFALSE;
		}

		nn = AllocFbxNotifyNode();
		if (nn == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}

		nn->nr = notify;
		nn->entry = e;

		notify->nr_notifynode = (IPTR)nn;
		AddTail((struct List *)&e->notifylist, (struct Node *)&nn->chain);

		if (notify->nr_Flags & NRF_NOTIFY_INITIAL) {
			FbxDoNotifyRequest(fs, notify);
		}
	}

	notify->nr_Handler = fs->fsport;

	AddTail((struct List *)&fs->currvol->notifylist, (struct Node *)&nn->volumechain);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxRemoveNotify(struct FbxFS *fs, struct NotifyRequest *nr) {
	struct Library *SysBase = fs->sysbase;
	struct FbxNotifyNode *nn;

	PDEBUGF("action_rem_notify(%#p, %#p)\n", fs, nr);

	if (nr->nr_Handler != fs->fsport) {
		fs->r2 = 0;
		return DOSFALSE;
	}

	nn = (struct FbxNotifyNode *)nr->nr_notifynode;

	Remove((struct Node *)&nn->chain);
	Remove((struct Node *)&nn->volumechain);

	if (nn->entry != NULL) {
		FbxCleanupEntry(fs, nn->entry);
	}

	FreeFbxNotifyNode(nn);

	nr->nr_MsgCount = 0;
	nr->nr_notifynode = (IPTR)NULL;
	return DOSTRUE;
}

static void FbxFillInfoData(struct FbxFS *fs, struct InfoData *info) {
	struct FbxVolume *vol = fs->currvol;

	bzero(info, sizeof(*info));

	info->id_UnitNumber = fs->fssm ? fs->fssm->fssm_Unit : -1;

	if (OKVOLUME(vol)) {
		struct statvfs st;

		bzero(&st, sizeof(st));

		Fbx_statfs(fs, "/", &st);

		if (vol->writeprotect || (st.f_flag & ST_RDONLY))
			info->id_DiskState = ID_WRITE_PROTECTED;
		else
			info->id_DiskState = ID_VALIDATED;

		info->id_NumBlocks = st.f_blocks;
		info->id_NumBlocksUsed = st.f_blocks - st.f_bfree;
		info->id_BytesPerBlock = st.f_frsize;
		info->id_DiskType = fs->dostype;
		info->id_VolumeNode = MKBADDR(vol);

		if (!IsMinListEmpty(&vol->locklist) ||
			!IsMinListEmpty(&vol->notifylist) ||
			!IsMinListEmpty(&vol->unres_notifys))
		{
			info->id_InUse = DOSTRUE;
		}
	}
	else
	{
		info->id_DiskState     = ID_VALIDATING;
		info->id_BytesPerBlock = 512;

		if (BADVOLUME(vol))
		{
			//info->id_NumSoftErrors = -1;
			info->id_DiskType       = ID_NOT_REALLY_DOS;
		}
		else if (fs->inhibit)
		{
			info->id_DiskType       = ID_BUSY_DISK;
			info->id_InUse         = DOSTRUE;
		}
		else
		{
			info->id_DiskType       = ID_NO_DISK_PRESENT;
		}
	}
}

static int FbxDiskInfo(struct FbxFS *fs, struct InfoData *info) {
	PDEBUGF("FbxDiskInfo(%#p, %#p)\n", fs, info);

	FbxFillInfoData(fs, info);

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxInfo(struct FbxFS *fs, struct FbxLock *lock, struct InfoData *info) {
	struct FbxVolume *vol;

	PDEBUGF("FbxInfo(%#p, %#p, %#p)\n", fs, lock, info);

	CHECKVOLUME(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);
		vol = lock->fsvol;
	} else {
		vol = fs->currvol;
	}

	if (vol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	FbxFillInfoData(fs, info);

	fs->r2 = 0;
	return DOSTRUE;
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

static int FbxInhibit(struct FbxFS *fs, int inhibit) {
	PDEBUGF("FbxInhibit(%#p, %d)\n", fs, inhibit);

	if (inhibit) {
		if (fs->inhibit == 0) {
			FbxCleanupVolume(fs);
		}
		fs->inhibit++;
	} else {
		if (fs->inhibit) {
			fs->inhibit--;
			if (fs->inhibit == 0) {
				fs->dosetup = TRUE;
			}
		}
	}

	return DOSTRUE;
}

static int FbxWriteProtect(struct FbxFS *fs, int on_off, IPTR passkey) {
	PDEBUGF("FbxWriteProtect(%#p, %d, %#p)\n", fs, on_off, (APTR)passkey);

	CHECKVOLUME(DOSFALSE);

	if (on_off) { // protect?
		if (fs->currvol->writeprotect) {
			if (fs->currvol->passkey == 0 && passkey == 0) {
				fs->r2 = 0;
				return DOSTRUE;
			} else {
				fs->r2 = ERROR_WRITE_PROTECTED;
				return DOSFALSE;
			}
		} else {
			fs->currvol->writeprotect = TRUE;
			fs->currvol->passkey = passkey;
			FbxFlushAll(fs);
		}
	} else { // unprotect
		if (fs->currvol->writeprotect) {
			if (fs->currvol->passkey != 0 && fs->currvol->passkey != passkey) {
				fs->r2 = ERROR_BAD_NUMBER;
				return DOSFALSE;
			}
			fs->currvol->writeprotect = FALSE;
		}
	}
	fs->r2 = 0;
	return DOSTRUE;
}

static BPTR FbxCurrentVolume(struct FbxFS *fs, struct FbxLock *lock) {
	fs->r2 = fs->fssm ? fs->fssm->fssm_Unit : -1; // yeah..
	if (lock != NULL) {
		CHECKLOCK(lock, ZERO);
		return lock->volumebptr;
	} else {
		CHECKVOLUME(ZERO);
		return MKBADDR(fs->currvol);
	}
}

static int FbxFormat(struct FbxFS *fs, const char *volname, ULONG dostype) {
	int error;

	PDEBUGF("FbxFormat(%#p, '%s', %#lx)\n", fs, volname, dostype);

	if (!fs->inhibit) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	CHECKSTRING(volname, DOSFALSE);

	error = Fbx_format(fs, volname, dostype);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	fs->r2 = 0;
	return DOSTRUE;
}

static int FbxRelabel(struct FbxFS *fs, const char *volname) {
	struct FbxVolume *vol = fs->currvol;
	int error;

	PDEBUGF("FbxRelabel(%#p, '%s')\n", fs, volname);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	CHECKSTRING(volname, DOSFALSE);

	error = Fbx_relabel(fs, volname);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxAsyncRenameVolume(fs, vol, volname);

	FbxNotifyDiskChange(fs, IECLASS_DISKINSERTED);

	fs->r2 = 0;
	return DOSTRUE;
}

SIPTR FbxDoPacket(struct FbxFS *fs, struct DosPacket *pkt) {
	LONG type;
	SIPTR r1;
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	#define BTOC(arg) ((const char *)(arg))
	#define BTOC2(arg) ((const char *)(arg))
#else
	char namebuf[256], namebuf2[256];
	#define BTOC(arg) ({ \
		CopyStringBSTRToC((arg), namebuf, sizeof(namebuf)); \
		namebuf;})
	#define BTOC2(arg) ({ \
		CopyStringBSTRToC((arg), namebuf2, sizeof(namebuf2)); \
		namebuf2;})
#endif

	PDEBUGF("FbxDoPacket(%#p, %#p)\n", fs, pkt);

#ifndef NODEBUG
	struct Task *callertask = pkt->dp_Port->mp_SigTask;
	PDEBUGF("action %ld task %#p '%s'\n", pkt->dp_Type, callertask, callertask->tc_Node.ln_Name);
#endif

	type = pkt->dp_Type;
	switch (type) {
	case ACTION_FINDUPDATE:
		r1 = FbxOpenFile(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			MODE_READWRITE);
		break;
	case ACTION_FINDINPUT:
		r1 = FbxOpenFile(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			MODE_OLDFILE);
		break;
	case ACTION_FINDOUTPUT:
		r1 = FbxOpenFile(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			MODE_NEWFILE);
		break;
	case ACTION_READ:
		r1 = FbxReadFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(APTR)pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_WRITE:
		r1 = FbxWriteFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(CONST_APTR)pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_SEEK:
		r1 = FbxSeekFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_END:
		r1 = FbxCloseFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_SET_FILE_SIZE:
		r1 = FbxSetFileSize(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_LOCATE_OBJECT:
		r1 = (SIPTR)MKBADDR(FbxLocateObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			BTOC(pkt->dp_Arg2), pkt->dp_Arg3));
		break;
	case ACTION_COPY_DIR:
	case ACTION_COPY_DIR_FH:
		r1 = (SIPTR)MKBADDR(FbxDupLock(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1)));
		break;
	case ACTION_FREE_LOCK:
		r1 = FbxUnLockObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_EXAMINE_OBJECT:
	case ACTION_EXAMINE_FH:
		r1 = FbxExamineObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(struct FileInfoBlock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_EXAMINE_NEXT:
		r1 = FbxExamineNext(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(struct FileInfoBlock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_CREATE_DIR:
		r1 = (SIPTR)MKBADDR(FbxCreateDir(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			BTOC(pkt->dp_Arg2)));
		break;
	case ACTION_DELETE_OBJECT:
		r1 = FbxDeleteObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2));
		break;
	case ACTION_RENAME_OBJECT:
		r1 = FbxRenameObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2),
			(struct FbxLock *)BADDR(pkt->dp_Arg3), BTOC2(pkt->dp_Arg4));
		break;
	case ACTION_PARENT:
	case ACTION_PARENT_FH:
		r1 = (SIPTR)MKBADDR(FbxLocateParent(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1)));
		break;
	case ACTION_SET_PROTECT:
		r1 = FbxSetProtection(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			pkt->dp_Arg4);
		break;
	case ACTION_SET_COMMENT:
		r1 = FbxSetComment(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			BTOC2(pkt->dp_Arg4));
		break;
	case ACTION_SET_DATE:
		r1 = FbxSetDate(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			(CONST_APTR)pkt->dp_Arg4);
		break;
	case ACTION_FH_FROM_LOCK:
		r1 = FbxOpenLock(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_SAME_LOCK:
		r1 = FbxSameLock(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_MAKE_LINK:
		if (pkt->dp_Arg4 == LINK_HARD) {
			r1 = FbxMakeHardLink(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2),
				(struct FbxLock *)BADDR(pkt->dp_Arg3));
		} else {
			r1 = FbxMakeSoftLink(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2),
				(const char *)pkt->dp_Arg3);
		}
		break;
	case ACTION_READ_LINK:
		r1 = FbxReadLink(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(const char *)pkt->dp_Arg2, (char *)pkt->dp_Arg3, pkt->dp_Arg4);
		break;
	case ACTION_CHANGE_MODE:
		if (pkt->dp_Arg1 == CHANGE_FH) {
			struct FileHandle *fh = BADDR(pkt->dp_Arg2);
			r1 = FbxChangeMode(fs, (struct FbxLock *)BADDR(fh->fh_Arg1), pkt->dp_Arg3);
		} else {
			r1 = FbxChangeMode(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), pkt->dp_Arg3);
		}
		break;
	case ACTION_EXAMINE_ALL:
		r1 = FbxExamineAll(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), (APTR)pkt->dp_Arg2,
			pkt->dp_Arg3, pkt->dp_Arg4, (struct ExAllControl *)pkt->dp_Arg5);
		break;
	case ACTION_EXAMINE_ALL_END:
		r1 = FbxExamineAllEnd(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), (APTR)pkt->dp_Arg2,
			pkt->dp_Arg3, pkt->dp_Arg4, (struct ExAllControl *)pkt->dp_Arg5);
		break;
	case ACTION_ADD_NOTIFY:
		r1 = FbxAddNotify(fs, (struct NotifyRequest *)pkt->dp_Arg1);
		break;
	case ACTION_REMOVE_NOTIFY:
		r1 = FbxRemoveNotify(fs, (struct NotifyRequest *)pkt->dp_Arg1);
		break;
	case ACTION_CURRENT_VOLUME:
		r1 = (SIPTR)FbxCurrentVolume(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_DISK_INFO:
		r1 = FbxDiskInfo(fs, (struct InfoData *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_INFO:
		r1 = FbxInfo(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), (struct InfoData *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_INHIBIT:
		r1 = FbxInhibit(fs, pkt->dp_Arg1);
		break;
	case ACTION_WRITE_PROTECT:
		r1 = FbxWriteProtect(fs, pkt->dp_Arg1, pkt->dp_Arg2);
		break;
	case ACTION_FORMAT:
		r1 = FbxFormat(fs, BTOC(pkt->dp_Arg1), pkt->dp_Arg2);
		break;
	case ACTION_RENAME_DISK:
		r1 = FbxRelabel(fs, BTOC(pkt->dp_Arg1));
		break;
	case ACTION_SET_OWNER:
		r1 = FbxSetOwnerInfo(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			pkt->dp_Arg4 >> 16, pkt->dp_Arg4 & 0xffff);
		break;
	case ACTION_IS_FILESYSTEM:
		r1 = DOSTRUE;
		fs->r2 = 0;
		break;
	case 0:
		r1 = DOSFALSE;
		fs->r2 = 0;
		break;
	default:
		r1 = DOSFALSE;
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		break;
	}

	PDEBUGF("Done with packet %#p. r1 %#p r2 %#p\n\n", pkt, (APTR)r1, (APTR)fs->r2);

	return r1;
}

void FbxHandleNotifyReplies(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct NotifyMessage *nm;
	struct NotifyRequest *nr;

	NDEBUGF("FbxHandleNotifyReplies(%#p)\n", fs);

	while ((nm = (struct NotifyMessage *)GetMsg(fs->notifyreplyport)) != NULL) {
		nr = nm->nm_NReq;
		if (nr->nr_Flags & NRF_MAGIC) {
			// reuse request and send it one more time
			nr->nr_Flags &= ~NRF_MAGIC;
			PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *)nm);
		} else {
			nr->nr_MsgCount--;
			FreeNotifyMessage(nm);
		}
	}
}

void FbxNotifyDiskChange(struct FbxFS *fs, UBYTE ieclass) {
	struct Library *SysBase = fs->sysbase;
	struct MsgPort *inputmp;
	struct IOStdReq *inputio;
	struct InputEvent ie;
	struct timeval tv;

	DEBUGF("FbxNotifyDiskChange(%#p, %#x)\n", fs, class);

	inputmp = CreateMsgPort();
	inputio = CreateIORequest(inputmp, sizeof(struct IOStdReq));
	if (inputio != NULL) {
		if (OpenDevice((CONST_STRPTR)"input.device", 0, (struct IORequest *)inputio, 0) == 0) {
			FbxGetUpTime(fs, &tv);

			ie.ie_NextEvent = 0;
			ie.ie_Class = ieclass;
			ie.ie_SubClass = 0;
			ie.ie_Code = 0;
			ie.ie_Qualifier = IEQUALIFIER_MULTIBROADCAST;
			ie.ie_EventAddress = 0;
			ie.ie_TimeStamp = tv;

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

struct timerequest *FbxSetupTimerIO(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct MsgPort *mp;
	struct timerequest *tr;

	DEBUGF("FbxSetupTimerIO(%#p)\n", fs);

	mp = CreateMsgPort();
	tr = CreateIORequest(mp, sizeof(*tr));
	if (tr == NULL) {
		DeleteMsgPort(mp);
		return NULL;
	}

	if (OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) != 0) {
		DeleteIORequest(tr);
		DeleteMsgPort(mp);
		return NULL;
	}

	fs->timerio   = tr;
	fs->timerbase = tr->tr_node.io_Device;
	fs->timerbusy = FALSE;

	return tr;
}

void FbxCleanupTimerIO(struct FbxFS *fs) {
	DEBUGF("FbxCleanupTimerIO(%#p)\n", fs);

	if (fs->timerbase != NULL) {
		struct Library *SysBase = fs->sysbase;
		struct timerequest *tr = fs->timerio;
		struct MsgPort *mp = tr->tr_node.io_Message.mn_ReplyPort;

		CloseDevice((struct IORequest *)tr);
		DeleteIORequest(tr);
		DeleteMsgPort(mp);
	}
}

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

