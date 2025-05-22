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
#include "fuse_stubs.h"

QUAD FbxGetFileSize(struct FbxFS *fs, struct FbxLock *lock) {
	struct fbx_stat statbuf;
	int error;

	PDEBUGF("FbxGetFileSize(%#p, %#p)\n", fs, lock);

	CHECKVOLUME(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	error = Fbx_fgetattr(fs, lock->entry->path, &statbuf, lock->info);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	fs->r2 = 0;
	return statbuf.st_size;
}

