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

int FbxDie(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct MinNode *chain;
	struct Message *msg;

	/* check if shutdown is already in progress */
	if (fs->shutdown) {
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		return DOSFALSE;
	}

	if (fs->devnode != NULL) {
		fs->devnode->dn_Task = NULL;
		fs->devnode = NULL;
	}

	FbxCleanupVolume(fs);

	while ((chain = (struct MinNode *)RemHead((struct List *)&fs->volumelist)) != NULL) {
		struct FbxVolume *vol = FSVOLUMEFROMFSCHAIN(chain);

		while ((chain = (struct MinNode *)RemHead((struct List *)&vol->locklist)) != NULL) {
			struct FbxLock *lock = FSLOCKFROMVOLUMECHAIN(chain);

			Remove((struct Node *)&lock->entrychain);
			FbxCleanupEntry(fs, lock->entry);
			lock->entry = NULL;

			lock->fs = NULL; /* invalidate lock */

			/* if (lock->fh != NULL) {
				FbxCollectFH(fs, lock->fh);

				FreeFbxLock(lock);
			} else */ {
				FbxCollectLock(fs, lock);
			}
		}

		while ((chain = (struct MinNode *)RemHead((struct List *)&vol->notifylist)) != NULL) {
			struct FbxNotifyNode *nn = FSNOTIFYNODEFROMVOLUMECHAIN(chain);

			Remove((struct Node *)&nn->chain);
			if (nn->entry != NULL) {
				FbxCleanupEntry(fs, nn->entry);
				nn->entry = NULL;
			}

			FbxCollectNotifyNode(fs, nn);
		}

		FreeFbxVolume(vol);
	}

	/* Redirect remaining packets to the lock handler process */
	while ((msg = GetMsg(fs->fsport)) != NULL) {
		PutMsg(fs->lhproc_port, msg);
	}

	fs->shutdown = TRUE;

	fs->r2 = 0;
	return DOSTRUE;
}

