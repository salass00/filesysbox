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

static int Fbx_link(struct FbxFS *fs, const char *dest, const char *path)
{
	ODEBUGF("Fbx_link(%#p, '%s', '%s')\n", fs, dest, path);

	return FSOP link(dest, path, &fs->fcntx);
}

int FbxMakeHardLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	struct FbxLock *lock2)
{
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

