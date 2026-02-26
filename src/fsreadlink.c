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
#include <string.h>

int FbxReadLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	char *buffer, int size)
{
	struct fbx_stat statbuf;
	int error, len;
	char fullpath[FBX_MAX_PATH];
	char softname[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
#endif

	PDEBUGF("FbxReadLink(%#p, %#p, '%s', %#p, %d)\n", fs, lock, name, buffer, size);

	CHECKVOLUME(-1);

	if (lock != NULL) {
		CHECKLOCK(lock, -1);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return -1;
		}
	}

#ifdef ENABLE_CHARSET_CONVERSION
	if (FbxLocalToUTF8(fs, fsname, name, FBX_MAX_NAME) >= FBX_MAX_NAME) {
		fs->r2 = ERROR_LINE_TOO_LONG;
		return -1;
	}
	name = fsname;
#else
	CHECKSTRING(name, -1);
#endif

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return -1;
	}

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	// check if it's a soft link
	if (!S_ISLNK(statbuf.st_mode)) {
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return -1;
	}

	error = Fbx_readlink(fs, fullpath, softname, FBX_MAX_PATH);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

#ifdef ENABLE_CHARSET_CONVERSION
	len = FbxUTF8ToLocal(fs, buffer, softname, size);
#else
	len = FbxStrlcpy(fs, buffer, softname, size);
#endif
	if (len >= size) {
		fs->r2 = ERROR_LINE_TOO_LONG;
		return -2;
	}

	fs->r2 = 0;
	return len;
}

