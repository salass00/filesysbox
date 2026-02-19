/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include "fuse_stubs.h"

static int Fbx_mkdir(struct FbxFS *fs, const char *path, mode_t mode)
{
	ODEBUGF("Fbx_mkdir(%#p, '%s', 0%o)\n", fs, path, mode);

	return FSOP mkdir(path, mode, &fs->fcntx);
}

struct FbxLock *FbxCreateDir(struct FbxFS *fs, struct FbxLock *lock, const char *name) {
	struct FbxEntry *e;
	int error;
	struct FbxLock *lock2;
	struct fbx_stat statbuf;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
#endif

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

#ifdef ENABLE_CHARSET_CONVERSION
	if (FbxLocalToUTF8(fs, fsname, name, FBX_MAX_NAME) >= FBX_MAX_NAME) {
		fs->r2 = ERROR_LINE_TOO_LONG;
		return NULL;
	}
	name = fsname;
#else
	CHECKSTRING(name, NULL);
#endif

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

