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

int FbxSetComment(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const char *comment)
{
	char fullpath[FBX_MAX_PATH];
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

