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
#include <devices/inputevent.h>

static int Fbx_relabel(struct FbxFS *fs, const char *volname)
{
	ODEBUGF("Fbx_relabel(%#p, '%s')\n", fs, volname);

	return FSOP relabel(volname, &fs->fcntx);
}

int FbxRelabel(struct FbxFS *fs, const char *volname) {
	struct FbxVolume *vol = fs->currvol;
	int error;

	PDEBUGF("FbxRelabel(%#p, '%s')\n", fs, volname);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	CHECKSTRING(volname, DOSFALSE);

	error = Fbx_relabel(fs, volname);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxAsyncRenameVolume(fs, vol, volname);

	FbxNotifyDiskChange(fs, IECLASS_DISKINSERTED);

	fs->r2 = 0;
	return DOSTRUE;
}

