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

static int Fbx_unlink(struct FbxFS *fs, const char *path)
{
	ODEBUGF("Fbx_unlink(%#p, '%s')\n", fs, path);

	return FSOP unlink(path, &fs->fcntx);
}

static int Fbx_rmdir(struct FbxFS *fs, const char *path)
{
	ODEBUGF("Fbx_rmdir(%#p, '%s')\n", fs, path);

	return FSOP rmdir(path, &fs->fcntx);
}

int FbxDeleteObject(struct FbxFS *fs, struct FbxLock *lock, const char *name) {
	char fullpath[FBX_MAX_PATH];
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

	if (strcmp(fullpath, "/") == 0) {
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

