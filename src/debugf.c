/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef NODEBUG
#include "filesysbox_internal.h"
#include <clib/debug_protos.h>

int vdebugf(const char *fmt, va_list args) {
	char buffer[256];

	int retval = vsnprintf(buffer, sizeof(buffer), fmt, args);
	KPutStr((CONST_STRPTR)buffer);

	return retval;
}

int debugf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int retval = vdebugf(fmt, ap);
	va_end(ap);
	return retval;
}
#endif /* !NODEBUG */

