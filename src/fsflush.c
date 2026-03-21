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

static int Fbx_fsync(struct FbxFS *fs, const char *path, int x, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_fsync(%p, '%s', %d, %p)\n", fs, path, x, fi);

	return FSOP fsync(path, x, fi, &fs->fcntx);
}

int FbxFlushAll(struct FbxFS *fs) {
	PDEBUGF("FbxFlushAll(%p)\n", fs);

	if (OKVOLUME(fs->currvol)) {
		Fbx_fsync(fs, "/", 0, NULL);
	}

	/* reset update timeouts */
	FbxSetModifyState(fs, 0);

	fs->r2 = 0;
	return DOSTRUE;
}

