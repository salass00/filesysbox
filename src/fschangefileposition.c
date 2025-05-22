/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifdef ENABLE_DP64_SUPPORT
#include "filesysbox_internal.h"

int FbxChangeFilePosition(struct FbxFS *fs, struct FbxLock *lock, QUAD pos, int mode) {
	QUAD newpos, oldpos, size;

	PDEBUGF("FbxChangeFilePosition(%#p, %#p, %lld, %d)\n", fs, lock, pos, mode);

	oldpos = FbxGetFilePosition(fs, lock);
	if (oldpos == -1) return DOSFALSE;

	size = FbxGetFileSize(fs, lock);
	if (size == -1) return DOSFALSE;

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
		return DOSFALSE;
	}

	if (newpos < 0 || newpos > size) {
		fs->r2 = ERROR_SEEK_ERROR;
		return DOSFALSE;
	}

	lock->filepos = newpos;
	fs->r2 = 0;
	return DOSTRUE;
}

#endif /* #ifdef ENABLE_DP64_SUPPORT */

