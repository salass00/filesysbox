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
	if (FbxSeekFile64(fs, lock, pos, mode) == -1) {
		return DOSFALSE;
	}

	return DOSTRUE;
}

#endif /* #ifdef ENABLE_DP64_SUPPORT */

