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

int FbxDie(struct FbxFS *fs) {
	struct FbxVolume *vol = fs->currvol;

	/* check if shutdown is already in progress */
	if (fs->shutdown) {
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		return DOSFALSE;
	}

	if (OKVOLUME(vol))
	{
		if (!IsMinListEmpty(&vol->locklist) ||
			!IsMinListEmpty(&vol->notifylist))
		{
			fs->r2 = ERROR_OBJECT_IN_USE;
			return DOSFALSE;
		}
	}

	if (!IsMinListEmpty(&fs->volumelist))
	{
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	if (fs->devnode != NULL) {
		fs->devnode->dn_Task = NULL;
		fs->devnode = NULL;
	}

	FbxCleanupVolume(fs);

	fs->shutdown = TRUE;

	fs->r2 = 0;
	return DOSTRUE;
}

