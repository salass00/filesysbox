/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef NODEBUG
#include "filesysbox_internal.h"
//#include <clib/debug_protos.h>

extern struct Library *SysBase;

int debug_putc_cb(char ch, void *udata) {
#ifdef __AROS__
	//char tmp[4];
	//tmp[0] = ch;
	//tmp[1] = '\0';
	//KPutStr((CONST_STRPTR)tmp);
	struct Library *SysBase = udata;
	RawPutChar(ch);
#else
	//KPutChar((unsigned char)ch);
	register char _d0 __asm__("d0") = ch;
	register struct Library *_a6 __asm__("a6") = udata;
	__asm__("jsr -516(a6)"
		:
		: "d" (_d0), "a" (_a6)
		: "d0", "d1", "a0", "a1", "cc"
	);
#endif
	return 0;
}

int vdebugf(const char *fmt, va_list args) {
	return FbxDoFmt(debug_putc_cb, SysBase, fmt, args);
}

int debugf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int retval = vdebugf(fmt, ap);
	va_end(ap);
	return retval;
}
#endif /* !NODEBUG */

