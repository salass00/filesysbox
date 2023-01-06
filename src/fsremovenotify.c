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

int FbxRemoveNotify(struct FbxFS *fs, struct NotifyRequest *nr) {
	struct Library *SysBase = fs->sysbase;
	struct FbxNotifyNode *nn;

	PDEBUGF("action_rem_notify(%#p, %#p)\n", fs, nr);

	if (nr->nr_Handler != fs->fsport) {
		fs->r2 = 0;
		return DOSFALSE;
	}

	nn = (struct FbxNotifyNode *)nr->nr_notifynode;

	Remove((struct Node *)&nn->chain);
	Remove((struct Node *)&nn->volumechain);

	if (nn->entry != NULL) {
		FbxCleanupEntry(fs, nn->entry);
	}

	FreeFbxNotifyNode(nn);

	nr->nr_MsgCount = 0;
	nr->nr_notifynode = (IPTR)NULL;
	return DOSTRUE;
}

