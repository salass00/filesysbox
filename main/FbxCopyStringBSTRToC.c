/*
 * Copyright (c) 2008-2011 Leif Salomonsson
 * Copyright (c) 2013-2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

#ifdef __AROS__
AROS_LH3(void, FbxCopyStringBSTRToC,
	AROS_LHA(BSTR, src, A0),
	AROS_LHA(STRPTR, dst, A1),
	AROS_LHA(ULONG, size, D0),
	struct FileSysBoxBase *, libBase, 16, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxCopyStringBSTRToC(
	REG(a0, BSTR src),
	REG(a1, STRPTR dst),
	REG(d0, ULONG size),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif

	CopyStringBSTRToC(src, (char *)dst, size);

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

