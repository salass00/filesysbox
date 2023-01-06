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

#ifndef __AROS__
static size_t strnlen(const char *str, size_t maxlen)
{
	const char *s = str;

	while (maxlen && *s++ != '\0') maxlen--;

	return (s - str);
}
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
	size_t len = strnlen(cstr, 255);
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

unsigned int FbxHashPath(struct FbxFS *fs, const char *str) {
	DEBUGF("FbxHashPath(%#p, '%s')\n", fs, str);

	if (fs->currvol->vflags & FBXVF_CASE_SENSITIVE)
		return FbxHashPathCase(fs, str);
	else
		return FbxHashPathNoCase(fs, str);
}

struct FbxEntry *FbxFindEntry(struct FbxFS *fs, const char *path) {
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

void FreeFbxDirData(struct FbxFS *fs, struct FbxDirData *dd) {
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

void FreeFbxDirDataList(struct FbxFS *fs, struct MinList *list) {
	struct MinNode *chain, *succ;

	DEBUGF("FreeFbxDirDataList(%#p, %#p)\n", fs, list);

	chain = list->mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		FreeFbxDirData(fs, FSDIRDATAFROMNODE(chain));
		chain = succ;
	}

	NEWLIST(list);
}

void FbxEndLock(struct FbxFS *fs, struct FbxLock *lock) {
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

void FbxAddEntry(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	ULONG i;

	DEBUGF("FbxAddEntry(%#p, %#p)\n", fs, e);

	i = FbxHashPath(fs, e->path);
	AddTail((struct List *)&fs->currvol->entrytab[i], (struct Node *)&e->hashchain);
}

BOOL FbxParentPath(struct FbxFS *fs, char *pathbuf) {
	if (strcmp(pathbuf, "/") == 0) {
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

static const char *FbxSkipColon(const char *s) {
	const char *s2 = strrchr(s, ':');
	return s2 ? (s2 + 1) : s;
}

BOOL FbxLockName2Path(struct FbxFS *fs, struct FbxLock *lock, const char *name, char *fullpathbuf) {
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
			if (strcmp(fullpathbuf, "/") != 0 && tname[0])
				FbxStrlcat(fs, fullpathbuf, "/", MAXPATHLEN);
			FbxStrlcat(fs, fullpathbuf, tname, MAXPATHLEN);
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

void FbxDoNotifyRequest(struct FbxFS *fs, struct NotifyRequest *nr) {
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

void FbxDoNotifyEntry(struct FbxFS *fs, struct FbxEntry *entry) {
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

void FbxDoNotify(struct FbxFS *fs, const char *path) {
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

void FbxSetEntryPath(struct FbxFS *fs, struct FbxEntry *e, const char *p) {
	FbxStrlcpy(fs, e->path, p, MAXPATHLEN);
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

void FbxCleanupEntry(struct FbxFS *fs, struct FbxEntry *e) {
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

void FbxTryResolveNotify(struct FbxFS *fs, struct FbxEntry *e) {
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

BOOL FbxCheckLock(struct FbxFS *fs, struct FbxLock *lock) {
	if (lock->fs != fs) {
		DEBUGF("INVALID LOCK %#p, -> cookie %#p (should be %#p)\n", lock, lock->fs, fs);
		return FALSE;
	}
	return TRUE;
}

ULONG FbxGetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath) {
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

void FbxUnResolveNotifys(struct FbxFS *fs, struct FbxEntry *e) {
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

BOOL FbxIsParent(struct FbxFS *fs, const char *parent, const char *child) {
	int plen = FbxStrlen(fs, parent);
	if (strcmp(parent, "/") == 0) plen = 0;
	if (FbxStrncmp(fs, parent, child, plen) == 0 && child[plen] == '/')
		return TRUE;
	else
		return FALSE;
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
	ds->ds_Tick = (sec % 60) * 50 + nsec / (1000*1000*1000/50);
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
		r1 = FbxExamineLock(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
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

