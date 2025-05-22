/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
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
	int error;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
	char fssoftname[FBX_MAX_PATH];
#endif

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

#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		if (FbxLocalToUTF8(fs, fsname, name, FBX_MAX_NAME) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		name = fsname;
		if (FbxLocalToUTF8(fs, fssoftname, softname, FBX_MAX_PATH) >= FBX_MAX_PATH) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		softname = fssoftname;
	}
#else
	CHECKSTRING(name, DOSFALSE);
	CHECKSTRING(softname, DOSFALSE);
#endif

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

