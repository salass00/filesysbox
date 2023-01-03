/*
 * Copyright (c) 2013-2018 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

extern struct Library *SysBase;

#ifndef __AROS__
APTR AllocVecPooled(APTR mempool, ULONG size) {
	ULONG *pmem;

	pmem = AllocPooled(mempool, sizeof(ULONG) + size);
	if (pmem != NULL)
		*pmem++ = size;

	return pmem;
}

void FreeVecPooled(APTR mempool, APTR ptr) {
	ULONG *pmem;
	ULONG size;

	if (ptr != NULL) {
		pmem = ptr;
		size = *--pmem;
		FreePooled(mempool, pmem, sizeof(ULONG) + size);
	}
}
#endif

