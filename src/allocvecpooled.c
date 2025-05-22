/*
 * Copyright (c) 2013-2025 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

#ifndef __AROS__
extern struct Library *SysBase;

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

