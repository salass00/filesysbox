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

static int Fbx_symlink(struct FbxFS *fs, const char *dest, const char *path)
{
	ODEBUGF("Fbx_symlink(%#p, '%s', '%s')\n", fs, path, dest);

	return FSOP symlink(dest, path, &fs->fcntx);
}

int FbxMakeSoftLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const char *softname)
{
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

