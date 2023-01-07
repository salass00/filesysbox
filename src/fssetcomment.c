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
	int error;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
	char fscomment[FBX_MAX_COMMENT];
#endif

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

#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		if (local_to_utf8(fsname, name, FBX_MAX_NAME, fs->maptable) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		name = fsname;
		if (local_to_utf8(fscomment, comment, FBX_MAX_COMMENT, fs->maptable) >= FBX_MAX_COMMENT) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		comment = fscomment;
	}
#else
	CHECKSTRING(name, DOSFALSE);
	CHECKSTRING(comment, DOSFALSE);
#endif

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

