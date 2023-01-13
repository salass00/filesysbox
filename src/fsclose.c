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
#include "fuse_stubs.h"
#include <string.h>

static void FbxClearArchiveFlags(struct FbxFS *fs, const char *fullpath) {
	char pathbuf[FBX_MAX_PATH];
	ULONG prot;

	FbxStrlcpy(fs, pathbuf, fullpath, FBX_MAX_PATH);
	do {
		prot = FbxGetAmigaProtectionFlags(fs, pathbuf);
		if (prot & FIBF_ARCHIVE) {
			prot &= ~FIBF_ARCHIVE;
			FbxSetAmigaProtectionFlags(fs, pathbuf, prot);
		}
	} while (FbxParentPath(fs, pathbuf) && strcmp(pathbuf, "/") != 0);
}

int FbxCloseFile(struct FbxFS *fs, struct FbxLock *lock) {
	struct Library *SysBase = fs->sysbase;
	struct FbxEntry *e;

	PDEBUGF("FbxCloseFile(%#p, %#p)\n", fs, lock);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	e = lock->entry;

	if (lock->fh == NULL) {
		// this lock should be ended with FbxUnLockObject()
		debugf("FbxCloseFile: lock %#p was never opened!\n", lock);
		fs->r2 = ERROR_INVALID_LOCK;
		return DOSFALSE;
	}

	if (lock->info != NULL) {
		Fbx_release(fs, e->path, lock->info);
		FreeFuseFileInfo(fs, lock->info);
		lock->info = NULL;
	}

	lock->fh = NULL;

	if (lock->fsvol == fs->currvol && (lock->flags & LOCKFLAG_MODIFIED)) {
		FbxClearArchiveFlags(fs, e->path);
		FbxDoNotify(fs, e->path);
		FbxSetModifyState(fs, 1);
	}

	FbxEndLock(fs, lock);
	FbxCleanupEntry(fs, e);

	fs->r2 = 0;
	return DOSTRUE;
}

