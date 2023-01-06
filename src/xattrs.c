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

ULONG FbxGetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath) {
	char buffer[4];
	int res, i;
	ULONG prot = 0;

	res = Fbx_getxattr(fs, fullpath, fs->xattr_amiga_protection, buffer, sizeof(buffer));
	if (res == 4) {
		for (i = 0; i < 4; i++) {
			switch (buffer[i]) {
			case 'h': prot |= FIBF_HOLD; break;
			case 's': prot |= FIBF_SCRIPT; break;
			case 'p': prot |= FIBF_PURE; break;
			case 'a': prot |= FIBF_ARCHIVE; break;
			}
		}
	}

	return prot;
}

int FbxSetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath, ULONG prot) {
	int error;

	if (prot & (FIBF_HOLD|FIBF_SCRIPT|FIBF_PURE|FIBF_ARCHIVE)) {
		char buffer[4];
		buffer[0] = (prot & FIBF_HOLD   ) ? 'h' : '-';
		buffer[1] = (prot & FIBF_SCRIPT ) ? 's' : '-';
		buffer[2] = (prot & FIBF_PURE   ) ? 'p' : '-';
		buffer[3] = (prot & FIBF_ARCHIVE) ? 'a' : '-';
		error = Fbx_setxattr(fs, fullpath, fs->xattr_amiga_protection, buffer, 4, 0);
	} else {
		error = Fbx_removexattr(fs, fullpath, fs->xattr_amiga_protection);
		if (error == -ENODATA)
			error = 0;
	}

	return error;
}

void FbxGetComment(struct FbxFS *fs, const char *fullpath, char *comment, size_t size) {
	int res;

	res = Fbx_getxattr(fs, fullpath, fs->xattr_amiga_comment, comment, size-1);
	if (res < 0) {
		comment[0] = '\0';
		return;
	}

	comment[min(res, size-1)] = '\0';
	if (!FbxCheckString(fs, comment)) {
		comment[0] = '\0';
		return;
	}
}

