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
#include <string.h>

#ifndef __AROS__
static size_t strnlen(const char *str, size_t maxlen)
{
	const char *s = str;

	while (maxlen && *s++ != '\0') maxlen--;

	return (s - str);
}
#endif

void CopyStringCToBSTR(const char *cstr, BSTR bstr, size_t size) {
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	strlcpy((char *)bstr, cstr, size);
#else
	UBYTE *dst = BADDR(bstr);
	size_t len = strnlen(cstr, 255);
	if (len >= size) len = size-1;
	*dst++ = len;
	memcpy(dst, cstr, len);
#endif
}

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

