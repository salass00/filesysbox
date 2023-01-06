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

static int Fbx_readlink(struct FbxFS *fs, const char *path, char *buf, size_t buflen)
{
	ODEBUGF("Fbx_readlink(%#p, '%s', %#p, %lu)\n", fs, path, buf, buflen);

	return FSOP readlink(path, buf, buflen, &fs->fcntx);
}

int FbxReadLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	char *buffer, int len)
{
	char fullpath[FBX_MAX_PATH];
	char softname[FBX_MAX_PATH];
	struct fbx_stat statbuf;
	int error;

	PDEBUGF("FbxReadLink(%#p, %#p, '%s', %#p, %d)\n", fs, lock, name, buffer, len);

	CHECKVOLUME(-1);

	if (lock != NULL) {
		CHECKLOCK(lock, -1);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return -1;
		}
	}

	CHECKSTRING(name, -1);

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

	if (FbxStrlcpy(fs, buffer, softname, len) >= len) {
		fs->r2 = ERROR_LINE_TOO_LONG;
		return -2;
	}

	fs->r2 = 0;
	return strlen(softname);
}

