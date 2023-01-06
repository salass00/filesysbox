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

struct FbxLock *FbxDupLock(struct FbxFS *fs, struct FbxLock *lock) {
	PDEBUGF("FbxDupLock(%#x)\n", lock);

	CHECKVOLUME(NULL);

	if (lock != NULL) {
		CHECKLOCK(lock, NULL);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return NULL;
		}

		lock = FbxLockEntry(fs, lock->entry, SHARED_LOCK);
		if (lock == NULL) return NULL;
	} else {
		lock = FbxLocateObject(fs, NULL, "", SHARED_LOCK);
		if (lock == NULL) return NULL;
	}

	fs->r2 = 0;
	return lock;
}

