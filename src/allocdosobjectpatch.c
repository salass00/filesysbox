/*
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifdef ENABLE_EXD_SUPPORT
#include "filesysbox_internal.h"

#include <exec/alerts.h>
#include <clib/debug_protos.h>

extern UBYTE binary_allocdosobject_bin_start[];
extern UBYTE binary_allocdosobject_bin_end[];

struct AllocDosObjectPatch {
	UWORD           FbxAllocDosObjectEntry;
	UWORD           FbxFreeDosObjectEntry;
	APTR            SysAllocDosObject;
	APTR            SysFreeDosObject;
	struct Library *SysBase;
	struct Library *UtilityBase;
	/* Code starts here */
};

#define LVOAllocDosObject (-228)
#define LVOFreeDosObject  (-234)

BOOL PatchAllocDosObject(struct FileSysBoxBase *fb) {
	struct Library *SysBase = fb->sysbase;
	struct Library *DOSBase = fb->dosbase;
	struct ExamineData *exd;
	struct AllocDosObjectPatch *patch;
	ULONG patch_size;
	APTR FbxAllocDosObject;
	APTR FbxFreeDosObject;

	/* Check if DOS_EXAMINEDATA is already supported */
	exd = AllocDosObject(DOS_EXAMINEDATA, NULL);
	if (exd != NULL) {
		ADEBUGF("AllocDosObject() already patched?\n");
		FreeDosObject(DOS_EXAMINEDATA, exd);
		return TRUE;
	}

	ADEBUGF("IoErr()=%ld\n", IoErr());

	patch_size = (ULONG)(binary_allocdosobject_bin_end - binary_allocdosobject_bin_start);
	ADEBUGF("patch size=%lu\n", patch_size);
	patch = AllocMem(patch_size, MEMF_PUBLIC);
	if (patch == NULL) {
		Alert(AG_NoMemory);
		return FALSE;
	}

	CopyMem(binary_allocdosobject_bin_start, patch, patch_size);

	patch->SysBase     = SysBase;
	patch->UtilityBase = fb->utilitybase;

	CacheClearE(patch, patch_size, CACRF_ClearI);

	FbxAllocDosObject = (APTR)((UBYTE *)patch + patch->FbxAllocDosObjectEntry);
	FbxFreeDosObject  = (APTR)((UBYTE *)patch + patch->FbxFreeDosObjectEntry);

	Forbid();
	patch->SysAllocDosObject = SetFunction(DOSBase, LVOAllocDosObject, FbxAllocDosObject);
	patch->SysFreeDosObject  = SetFunction(DOSBase, LVOFreeDosObject,  FbxFreeDosObject);
	Permit();

	ADEBUGF("AllocDosObject() and FreeDosObject() are now patched\n");

	return TRUE;
}

#endif /* ENABLE_EXD_SUPPORT */

