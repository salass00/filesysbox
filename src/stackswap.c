/*
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#if defined(ENABLE_STACKSWAP) && defined(ENABLE_C_STACKSWAP)
#include <libraries/filesysbox.h>
#include "filesysbox_vectors.h"
#include "filesysbox_internal.h"

#define FBX_MIN_STACK 16384

static inline IPTR FbxGetStackSize(struct Library *SysBase) {
	struct Task *me = FindTask(NULL);
	UBYTE *lower = me->tc_SPLower;
	UBYTE *upper = me->tc_SPUpper;
	UBYTE *sp = (UBYTE *)&me;

	if (sp >= lower && sp < upper)
		return (IPTR)(sp - lower);
	else
		return 0;
}

#ifdef __AROS__
AROS_LH1(LONG, FbxEventLoop_SS,
	AROS_LHA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 7, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
LONG FbxEventLoop_SS(
	REG(a0, struct FbxFS *fs),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	register struct Library *SysBase;
	register struct StackSwapStruct *sss;
	register LONG rc;
#ifdef __AROS__
	struct StackSwapArgs ssa;
#endif

	if (fs == NULL)
		return -1;

	SysBase = fs->sysbase;
	sss = &fs->stackswap;

	if (FbxGetStackSize(SysBase) >= (FBX_MIN_STACK - 16)) {
		return FbxEventLoop(fs, libBase);
	}

	sss->stk_Lower = AllocMem(FBX_MIN_STACK, MEMF_ANY);
	if (sss->stk_Lower == NULL) {
		Alert(AG_NoMemory);
		return -1;
	}
#ifdef __AROS__
	sss->stk_Upper = (UBYTE *)sss->stk_Lower + FBX_MIN_STACK;
	sss->stk_Pointer = sss->stk_Upper;
#else
	sss->stk_Upper = (IPTR)((UBYTE *)sss->stk_Lower + FBX_MIN_STACK);
	sss->stk_Pointer = (APTR)sss->stk_Upper;
#endif

#ifdef __AROS__
	ssa.Args[0] = (IPTR)fs;
	ssa.Args[1] = (IPTR)libBase;
	rc = (LONG)NewStackSwap(sss, FbxEventLoop, &ssa);
#else
	/* NOTE: This section can easily break if the compiler generates code
	 * that accesses the stack. This is why there is a special assembler
	 * version for the m68k-amigaos target.
	 */
	StackSwap(sss);
	rc = FbxEventLoop(fs, libBase);
	StackSwap(sss);
#endif

	FreeMem(sss->stk_Lower, FBX_MIN_STACK);

	return rc;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#endif /* defined(ENABLE_STACKSWAP) && defined(ENABLE_C_STACKSWAP) */
