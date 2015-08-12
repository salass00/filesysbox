/*
 * Copyright (c) 2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef __AROS__

#include "filesysbox_internal.h"

APTR FbxAllocVecPooled(struct FbxFS *fs, ULONG size) {
	ULONG *pmem;

	GetSysBase

	pmem = AllocPooled(fs->mempool, sizeof(ULONG) + size);
	if (pmem != NULL)
		*pmem++ = size;

	return pmem;
}

void FbxFreeVecPooled(struct FbxFS *fs, APTR ptr) {
	ULONG *pmem;
	ULONG size;

	GetSysBase

	if (ptr != NULL) {
		pmem = ptr;
		size = *--pmem;
		FreePooled(fs->mempool, pmem, sizeof(ULONG) + size);
	}
}

#endif

