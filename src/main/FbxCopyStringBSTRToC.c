/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

void CopyStringBSTRToC(BSTR bstr, char *cstr, size_t size) {
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	strlcpy(cstr, (const char *)bstr, size);
#else
	UBYTE *src = BADDR(bstr);
	size_t len = *src++;
	if (len >= size) len = size-1;
	memcpy(cstr, src, len);
	cstr[len] = '\0';
#endif
}

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

