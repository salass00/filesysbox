/*
 * Copyright (c) 2014-2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

#ifdef __AROS__
AROS_LH3(void, FbxSetSignalCallback,
	AROS_LHA(struct FbxFS *, fs, A0),
	AROS_LHA(FbxSignalCallbackFunc, func, A1),
	AROS_LHA(ULONG, signals, D0),
	struct FileSysBoxBase *, libBase, 12, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxSetSignalCallback(
	REG(a0, struct FbxFS *fs),
	REG(a1, FbxSignalCallbackFunc func),
	REG(d0, ULONG signals),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	ADEBUGF("FbxSetSignalCallback(%#p, %#p, 0x%lx)\n", fs, func, signals);

	if (fs != NULL) {
		fs->signalcallbackfunc = func;
		fs->signalcallbacksignals = signals;
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

