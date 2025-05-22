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

QUAD FbxGetFilePosition(struct FbxFS *fs, struct FbxLock *lock) {
	PDEBUGF("FbxGetFilePosition(%#p, %#p)\n", fs, lock);

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

	fs->r2 = 0;
	return lock->filepos;
}

