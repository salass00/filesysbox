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

struct FbxLock *FbxLocateObject(struct FbxFS *fs, struct FbxLock *lock,
	const char *name, int lockmode)
{
	struct fbx_stat statbuf;
	int error;
	LONG ntype;
	struct FbxEntry *e;
	struct FbxLock *lock2;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
#endif

	PDEBUGF("FbxLocateObject(%#p, %#p, '%s', %d)\n", fs, lock, name, lockmode);

	CHECKVOLUME(NULL);

	if (lock != NULL) {
		CHECKLOCK(lock, NULL);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return NULL;
		}
	}

#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		if (local_to_utf8(fsname, name, FBX_MAX_NAME, fs->maptable) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return NULL;
		}
		name = fsname;
	}
#else
	CHECKSTRING(name, NULL);
#endif

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

