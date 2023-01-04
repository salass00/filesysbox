/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

/****** filesysbox.library/FbxInstallTimerCallback **************************
*
*   NAME
*      FbxInstallTimerCallback -- Install timer callback. (V53.23)
*
*   SYNOPSIS
*      struct FbxTimerCallbackData * FbxInstallTimerCallback(
*          struct FbxFS * fs, FbxTimerCallbackFunc func, ULONG period);
*
*   FUNCTION
*       Adds a callback function that will get called as often as specified by
*       the period parameter.
*
*   INPUTS
*       fs - The result of FbxSetupFS().
*       func - Callback function.
*       period - Period value in milliseconds.
*
*   RESULT
*       A pointer to a private data structure that can be passed to
*       FbxUninstallTimerCallback() when the callback is no longer
*       needed or NULL for failure.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       FbxUninstallTimerCallback()
*       
*
*****************************************************************************
*
*/

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

			ObtainSemaphore(&fs->fssema);
			AddTail((struct List *)&fs->timercallbacklist, (struct Node *)&cb->fschain);
			ReleaseSemaphore(&fs->fssema);
		}
	}

	return cb;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

