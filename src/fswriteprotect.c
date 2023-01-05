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

int FbxWriteProtect(struct FbxFS *fs, int on_off, IPTR passkey) {
	PDEBUGF("FbxWriteProtect(%#p, %d, %#p)\n", fs, on_off, (APTR)passkey);

	CHECKVOLUME(DOSFALSE);

	if (on_off) { // protect?
		if (fs->currvol->writeprotect) {
			if (fs->currvol->passkey == 0 && passkey == 0) {
				fs->r2 = 0;
				return DOSTRUE;
			} else {
				fs->r2 = ERROR_WRITE_PROTECTED;
				return DOSFALSE;
			}
		} else {
			fs->currvol->writeprotect = TRUE;
			fs->currvol->passkey = passkey;
			FbxFlushAll(fs);
		}
	} else { // unprotect
		if (fs->currvol->writeprotect) {
			if (fs->currvol->passkey != 0 && fs->currvol->passkey != passkey) {
				fs->r2 = ERROR_BAD_NUMBER;
				return DOSFALSE;
			}
			fs->currvol->writeprotect = FALSE;
		}
	}
	fs->r2 = 0;
	return DOSTRUE;
}

