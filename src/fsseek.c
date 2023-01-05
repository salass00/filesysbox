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

QUAD FbxSeekFile(struct FbxFS *fs, struct FbxLock *lock, QUAD pos, int mode) {
	QUAD newpos, oldpos;
	int error;
	struct fbx_stat statbuf;

	PDEBUGF("FbxSeekFile(%#p, %#p, %lld, %d)\n", fs, lock, pos, mode);

	CHECKVOLUME(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	if (lock->info->nonseekable) {
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		return -1;
	}

	oldpos = lock->filepos;

	error = Fbx_fgetattr(fs, lock->entry->path, &statbuf, lock->info);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	switch (mode) {
	case OFFSET_BEGINNING:
		newpos = pos;
		break;
	case OFFSET_CURRENT:
		newpos = oldpos + pos;
		break;
	case OFFSET_END:
		newpos = statbuf.st_size + pos;
		break;
	default:
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	if (newpos < 0 || newpos > statbuf.st_size) {
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	lock->filepos = newpos;
	return oldpos;
}

