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
AROS_LH3(struct FbxTimerCallbackData *, FbxInstallTimerCallback,
	AROS_LHA(struct FbxFS *, fs, A0),
	AROS_LHA(FbxTimerCallbackFunc, func, A1),
	AROS_LHA(ULONG, period, D0),
	struct FileSysBoxBase *, libBase, 13, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
struct FbxTimerCallbackData *FbxInstallTimerCallback(
	REG(a0, struct FbxFS *fs),
	REG(a1, FbxTimerCallbackFunc func),
	REG(d0, ULONG period),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct FbxTimerCallbackData *cb = NULL;

	ADEBUGF("FbxInstallTimerCallback(%#p, %#p, %lu)\n", fs, func, period);

	if (fs != NULL && func != NULL && period != 0) {
		struct Library *SysBase = fs->sysbase;

		cb = AllocFbxTimerCallbackData(fs);
		if (cb != NULL) {
			cb->lastcall = FbxGetUpTimeMillis(fs);
			cb->period = period;
			cb->func = func;
			AddTail((struct List *)&fs->timercallbacklist, (struct Node *)&cb->fschain);
		}
	}

	return cb;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

