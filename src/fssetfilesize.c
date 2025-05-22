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

static int Fbx_ftruncate(struct FbxFS *fs, const char *path, QUAD size, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_ftruncate(%#p, '%s', %lld, %#p)\n", fs, path, size, fi);

	return FSOP ftruncate(path, size, fi, &fs->fcntx);
}

static QUAD FbxTruncNewSize(struct FbxFS *fs, struct FbxLock *lock, QUAD newsize) {
	struct FbxLock *fslock;
	struct MinNode *chain, *succ;

	PDEBUGF("FbxTruncNewSize(%#p, %#p, %lld)\n", fs, lock, newsize);

	chain = lock->entry->locklist.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		fslock = FSLOCKFROMENTRYCHAIN(chain);
		if (fslock != lock && fslock->filepos > newsize) {
			newsize = fslock->filepos;
		}
		chain = succ;
	}

	return newsize;
}

QUAD FbxSetFileSize64(struct FbxFS *fs, struct FbxLock *lock, QUAD offs, int mode) {
	QUAD oldsize, newsize;
	int error;

	PDEBUGF("FbxSetFileSize(%#p, %#p, %lld, %d)\n", fs, lock, offs, mode);

	oldsize = FbxGetFileSize(fs, lock);
	if (oldsize == -1) return -1;

	switch (mode) {
	case OFFSET_BEGINNING:
		newsize = offs;
		break;
	case OFFSET_CURRENT:
		newsize = lock->filepos + offs;
		break;
	case OFFSET_END:
		newsize = oldsize + offs;
		break;
	default:
		fs->r2 = ERROR_SEEK_ERROR;
		return -1;
	}

	if (newsize == oldsize) {
		fs->r2 = 0;
		return oldsize;
	}

	CHECKWRITABLE(-1);

	/* check if there are other locks to this file and
	 * if any of their positions are above newsize. */
	if (FbxTruncNewSize(fs, lock, newsize) > newsize) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return -1;
	}

	/* check if our lock's position points beyond new filesize
	 * if so, truncate position to new filesize.. */
	if (lock->filepos > newsize) lock->filepos = newsize;

	error = Fbx_ftruncate(fs, lock->entry->path, newsize, lock->info);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return -1;
	}

	lock->flags |= LOCKFLAG_MODIFIED; /* for notification */
	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return newsize;
}

SIPTR FbxSetFileSize(struct FbxFS *fs, struct FbxLock *lock, SIPTR offs, int mode) {
	QUAD newsize;

	newsize = FbxSetFileSize(fs, lock, offs, mode);
	if (newsize != (SIPTR)newsize) {
		fs->r2 = ERROR_OBJECT_TOO_LARGE;
		return -1;
	}

	return (SIPTR)newsize;
}

