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

int FbxUnLockObject(struct FbxFS *fs, struct FbxLock *lock) {
	struct FbxEntry *e;

	PDEBUGF("FbxUnLockObject(%#p, %#p)\n", fs, lock);

	if (lock == NULL) {
		debugf("FbxUnLockObject got a NULL lock, btw.\n");
		fs->r2 = 0;
		return DOSTRUE;
	}

	CHECKLOCK(lock, DOSFALSE);

	e = lock->entry;

	FbxEndLock(fs, lock);
	FbxCleanupEntry(fs, e);

	fs->r2 = 0;
	return DOSTRUE;
}

