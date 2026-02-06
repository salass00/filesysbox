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

int FbxInhibit(struct FbxFS *fs, int inhibit) {
	PDEBUGF("FbxInhibit(%#p, %d)\n", fs, inhibit);

	if (inhibit) {
		if (fs->inhibit == 0) {
			FbxCleanupVolume(fs);
		}
		fs->inhibit++;
	} else {
		if (fs->inhibit) {
			fs->inhibit--;
			if (fs->inhibit == 0) {
				fs->dosetup = TRUE;
			}
		}
	}

	return DOSTRUE;
}

