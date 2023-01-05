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

int FbxChangeMode(struct FbxFS *fs, struct FbxLock *lock, int mode) {
	PDEBUGF("FbxChangeMode(%#p, %#p, %d)\n", fs, lock, mode);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (!OneInMinList(&lock->entry->locklist)) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	lock->access = mode;
	if (mode == EXCLUSIVE_LOCK) {
		lock->entry->xlock = TRUE;
	} else if (mode == SHARED_LOCK) {
		lock->entry->xlock = FALSE;
	} else {
		fs->r2 = ERROR_BAD_NUMBER;
		return DOSFALSE;
	}

	fs->r2 = 0;
	return DOSTRUE;
}

