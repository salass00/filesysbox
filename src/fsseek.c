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

QUAD FbxSeekFile64(struct FbxFS *fs, struct FbxLock *lock, QUAD pos, int mode) {
	QUAD newpos, oldpos, size;

	PDEBUGF("FbxSeekFile(%#p, %#p, %lld, %d)\n", fs, lock, pos, mode);

	oldpos = FbxGetFilePosition(fs, lock);
	if (oldpos == -1) return -1;

	size = FbxGetFileSize(fs, lock);
	if (size == -1) return -1;

	switch (mode) {
	case OFFSET_BEGINNING:
		newpos = pos;
		break;
	case OFFSET_CURRENT:
		newpos = oldpos + pos;
		break;
	case OFFSET_END:
		newpos = size + pos;
		break;
	default:
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	if (newpos < 0 || newpos > size) {
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	lock->filepos = newpos;
	fs->r2 = 0;
	return oldpos;
}

SIPTR FbxSeekFile(struct FbxFS *fs, struct FbxLock *lock, SIPTR pos, int mode) {
	QUAD oldpos;

	oldpos = FbxSeekFile64(fs, lock, pos, mode);
	if (oldpos != (SIPTR)oldpos) {
		fs->r2 = ERROR_OBJECT_TOO_LARGE;
		return -1;
	}

	return (SIPTR)oldpos;
}

