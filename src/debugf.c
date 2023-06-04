/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef NODEBUG
#include "filesysbox_internal.h"
#include <clib/debug_protos.h>

int debug_putc_cb(char ch, void *udata) {
#ifdef __AROS__
	char tmp[4];
	tmp[0] = ch;
	tmp[1] = '\0';
	KPutStr((CONST_STRPTR)tmp);
#else
	KPutChar((unsigned char)ch);
#endif
	return 0;
}

int vdebugf(const char *fmt, va_list args) {
	return FbxDoFmt(debug_putc_cb, NULL, fmt, args);
}

int debugf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int retval = vdebugf(fmt, ap);
	va_end(ap);
	return retval;
}
#endif /* !NODEBUG */

