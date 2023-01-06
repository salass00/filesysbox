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

BPTR FbxCurrentVolume(struct FbxFS *fs, struct FbxLock *lock) {
	fs->r2 = fs->fssm ? fs->fssm->fssm_Unit : -1; // yeah..
	if (lock != NULL) {
		CHECKLOCK(lock, ZERO);
		return lock->volumebptr;
	} else {
		CHECKVOLUME(ZERO);
		return MKBADDR(fs->currvol);
	}
}

