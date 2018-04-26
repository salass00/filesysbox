/*
 * Copyright (c) 2008-2011 Leif Salomonsson
 * Copyright (c) 2013-2018 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

#ifdef __AROS__
AROS_LH0(LONG, FbxFuseVersion,
	struct FileSysBoxBase *, libBase, 10, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
LONG FbxFuseVersion(
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	ADEBUGF("FbxFuseVersion()\n");

	return FUSE_VERSION;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

