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

static int Fbx_chown(struct FbxFS *fs, const char *path, uid_t uid, gid_t gid)
{
	ODEBUGF("Fbx_chown(%#p, '%s', %#x, %#x)\n", fs, path, uid, gid);

	return FSOP chown(path, uid, gid, &fs->fcntx);
}

static uid_t FbxAmiga2UnixOwner(const UWORD owner) {
	if (owner == DOS_OWNER_ROOT) return (uid_t)0;
	else if (owner == DOS_OWNER_NONE) return (uid_t)-2;
	else return (uid_t)owner;
}

int FbxSetOwnerInfo(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	UWORD uid, UWORD gid)
{
	int error;
	char fullpath[FBX_MAX_PATH];

	PDEBUGF("FbxSetOwnerInfo(%#p, %#p, '%s', %#x, %#x)\n", fs, lock, name, uid, gid);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	CHECKSTRING(name, DOSFALSE);

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_chown(fs, fullpath, FbxAmiga2UnixOwner(uid), FbxAmiga2UnixOwner(gid));
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

