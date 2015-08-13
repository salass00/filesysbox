/*
 * Copyright (c) 2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#ifdef __AROS__
#include <clib/arossupport_protos.h>
#else
#include <clib/debug_protos.h>
#endif

#ifndef NODEBUG
int vdebugf(const char *fmt, va_list args) {
	char buffer[256];

	int retval = vsnprintf(buffer, sizeof(buffer), fmt, args);
	kprintf("%s", buffer);

	return retval;
}

int debugf(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int retval = vdebugf(fmt, ap);
	va_end(ap);
	return retval;
}
#endif


