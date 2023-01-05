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

static int Fbx_format(struct FbxFS *fs, const char *volname, ULONG dostype)
{
	ODEBUGF("Fbx_format(%#p, '%s', %#lx)\n", fs, volname, dostype);

	return FSOP format(volname, dostype, &fs->fcntx);
}

int FbxFormat(struct FbxFS *fs, const char *volname, ULONG dostype) {
	int error;

	PDEBUGF("FbxFormat(%#p, '%s', %#lx)\n", fs, volname, dostype);

	if (!fs->inhibit) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	CHECKSTRING(volname, DOSFALSE);

	error = Fbx_format(fs, volname, dostype);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	fs->r2 = 0;
	return DOSTRUE;
}

