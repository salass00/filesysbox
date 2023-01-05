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

int FbxSameLock(struct FbxFS *fs, struct FbxLock *lock, struct FbxLock *lock2) {
	PDEBUGF("FbxSameLock(%#p, %#p, %#p)\n", fs, lock, lock2);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);
	CHECKLOCK(lock2, DOSFALSE);

	fs->r2 = 0;
	if (lock == lock2) return DOSTRUE;
	if (lock->fsvol == lock2->fsvol) {
		struct FbxEntry *entry = lock->entry;
		struct FbxEntry *entry2 = lock2->entry;

		/* Check for same entries */
		if (entry == entry2)
			return DOSTRUE;

		/* Compare inodes (if valid) */
		if (fs->fsflags & FBXF_USE_INO)
		{
			if (entry->diskkey != 0 && entry->diskkey == entry2->diskkey)
				return DOSTRUE;
		}
	}
	return DOSFALSE;
}

