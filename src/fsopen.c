/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include "fuse_stubs.h"
#include <errno.h>

static int Fbx_create(struct FbxFS *fs, const char *path, mode_t mode, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_create(%#p, '%s', 0%o, %#p)\n", fs, path, mode, fi);

	return FSOP create(path, mode, fi, &fs->fcntx);
}

static int Fbx_mknod(struct FbxFS *fs, const char *path, mode_t mode, dev_t dev)
{
	ODEBUGF("Fbx_mknod(%#p, '%s', 0%o, %#x)\n", fs, path, mode, dev);

	return FSOP mknod(path, mode, dev, &fs->fcntx);
}

static int Fbx_truncate(struct FbxFS *fs, const char *path, QUAD size)
{
	ODEBUGF("Fbx_truncate(%#p, '%s', %lld)\n", fs, path, size);

	return FSOP truncate(path, size, &fs->fcntx);
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

int FbxOpenFile(struct FbxFS *fs, struct FileHandle *fh, struct FbxLock *lock,
	const char *name, int mode)
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

