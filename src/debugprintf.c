/*
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef NODEBUG
#include "filesysbox_internal.h"

#ifdef __AROS__
#include <clib/debug_protos.h>
#elif defined(__VBCC__)
void __KPutChar(__reg("d0") LONG, __reg("a6") struct Library *)="\tjsr\t-516(a6)";
#elif !defined(__GNUC__)
#include <clib/debug_protos.h>
#endif

int debug_putc_cb(char ch, void *udata) {
#ifdef __AROS__
	struct Library *SysBase = udata;
	RawPutChar(ch);
#elif defined(__GNUC__)
	register char _d0 __asm__("d0") = ch;
	register struct Library *_a6 __asm__("a6") = udata;
	__asm__("jsr -516(a6)"
		:
		: "d" (_d0), "a" (_a6)
		: "d0", "d1", "a0", "a1", "cc"
	);
#elif defined(__VBCC__)
	__KPutChar((UBYTE)ch, udata);
#elif
	KPutChar((UBYTE)ch);
#endif
	return 0;
}

size_t FbxDebugPrintf(const char *fmt, ...) {
	extern struct Library *SysBase;
	size_t retval;
	va_list ap;

	va_start(ap, fmt);
	retval = FbxDoFmt(debug_putc_cb, SysBase, fmt, ap);
	va_end(ap);

	return retval;
}

#endif /* !NODEBUG */

