/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include <string.h>

static int Fbx_open(struct FbxFS *fs, const char *path, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_open(%#p, '%s', %#p)\n", fs, path, fi);

	return FSOP open(path, fi, &fs->fcntx);
}

int FbxOpenLock(struct FbxFS *fs, struct FileHandle *fh, struct FbxLock *lock) {
	struct Library *SysBase = fs->sysbase;
	int error;

	PDEBUGF("FbxOpenLock(%#p, %#p, %#p)\n", fs, fh, lock);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (lock->entry->type != ETYPE_FILE) {
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	if (lock->info != NULL) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	lock->info = AllocFuseFileInfo(fs);
	if (lock->info == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return DOSFALSE;
	}
	memset(lock->info, 0, sizeof(*lock->info));

	if (fs->currvol->vflags & FBXVF_READ_ONLY)
		lock->info->flags = O_RDONLY;
	else
		lock->info->flags = O_RDWR;

	error = Fbx_open(fs, lock->entry->path, lock->info);
	if (error) {
		FreeFuseFileInfo(fs, lock->info);
		lock->info = NULL;
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	lock->fh = fh;
	fh->fh_Arg1 = (SIPTR)MKBADDR(lock);

	fs->r2 = 0;
	return DOSTRUE;
}

