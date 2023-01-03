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
AROS_LH3(void, FbxCopyStringCToBSTR,
	AROS_LHA(CONST_STRPTR, src, A0),
	AROS_LHA(BSTR, dst, A1),
	AROS_LHA(ULONG, size, D0),
	struct FileSysBoxBase *, libBase, 17, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxCopyStringCToBSTR(
	REG(a0, CONST_STRPTR src),
	REG(a1, BSTR dst),
	REG(d0, ULONG size),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif

	CopyStringCToBSTR((const char *)src, dst, size);

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

