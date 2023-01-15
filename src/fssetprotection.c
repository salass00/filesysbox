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
#include <errno.h>

static int Fbx_chmod(struct FbxFS *fs, const char *path, mode_t mode)
{
	ODEBUGF("Fbx_chmod(%#p, '%s', 0%o)\n", fs, path, mode);

	return FSOP chmod(path, mode, &fs->fcntx);
}

static mode_t FbxProtection2Mode(ULONG prot) {
	mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR;

	if (prot & FIBF_READ       ) mode &= ~(S_IRUSR);
	if (prot & FIBF_WRITE      ) mode &= ~(S_IWUSR);
	if (prot & FIBF_EXECUTE    ) mode &= ~(S_IXUSR);
	if (prot & FIBF_GRP_READ   ) mode |= S_IRGRP;
	if (prot & FIBF_GRP_WRITE  ) mode |= S_IWGRP;
	if (prot & FIBF_GRP_EXECUTE) mode |= S_IXGRP;
	if (prot & FIBF_OTR_READ   ) mode |= S_IROTH;
	if (prot & FIBF_OTR_WRITE  ) mode |= S_IWOTH;
	if (prot & FIBF_OTR_EXECUTE) mode |= S_IXOTH;

	return mode;
}

int FbxSetProtection(struct FbxFS *fs, struct FbxLock *lock, const char *name, ULONG prot) {
	int error;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
#endif

	PDEBUGF("FbxSetProtection(%#p, %#p, '%s', %#lx)\n", fs, lock, name, prot);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		if (FbxLocalToUTF8(fs, fsname, name, FBX_MAX_NAME) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		name = fsname;
	}
#else
	CHECKSTRING(name, DOSFALSE);
#endif

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_chmod(fs, fullpath, FbxProtection2Mode(prot));
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	error = FbxSetAmigaProtectionFlags(fs, fullpath, prot);
	if (error && (error != -ENOSYS && error != -EOPNOTSUPP)) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

