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

#define LONG_MAX (0x7fffffff)

struct FileSysStartupMsg *FbxObtainFSSM(struct FbxFS *fs) {
	if (fs->fssm == NULL) {
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return NULL;
	}

	/* Check for overflow */
	if (fs->fssmcount == LONG_MAX) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return NULL;
	}

	fs->fssmcount++;

	fs->r2 = 0;
	return fs->fssm;
}

int FbxReleaseFSSM(struct FbxFS *fs, struct FileSysStartupMsg *fssm) {
	if (fssm == NULL) {
		fs->r2 = 0;
		return DOSFALSE;
	}

	if (fssm == fs->fssm && fs->fssmcount != 0) {
		fs->fssmcount--;
	}

	fs->r2 = 0;
	return DOSTRUE;
}

