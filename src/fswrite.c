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

static int Fbx_write(struct FbxFS *fs, const char *path, const char *buf, size_t len,
	QUAD offset, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_write(%#p, '%s', %#p, %lu, %lld, %#p)\n", fs, path, buf, len, offset, fi);

	return FSOP write(path, buf, len, offset, fi, &fs->fcntx);
}

int FbxWriteFile(struct FbxFS *fs, struct FbxLock *lock, CONST_APTR buffer, int bytes) {
	int res;

	PDEBUGF("FbxWriteFile(%#p, %#p, %#p, %d)\n", fs, lock, buffer, bytes);

	CHECKVOLUME(-1);
	CHECKWRITABLE(-1);

	CHECKLOCK(lock, -1);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return -1;
	}

	if (bytes == 0) {
		fs->r2 = 0;
		return 0;
	}

	res = Fbx_write(fs, lock->entry->path, buffer, bytes, lock->filepos, lock->info);
	if (res < 0) {
		fs->r2 = FbxFuseErrno2Error(res);
		return -1;
	}

	if (res > 0) {
		lock->filepos += bytes;
		lock->flags |= LOCKFLAG_MODIFIED; // for notification
		FbxSetModifyState(fs, 1);
	}

	fs->r2 = (res == bytes) ? 0 : -1;
	return res;
}

