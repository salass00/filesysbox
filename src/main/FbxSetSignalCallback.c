/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "filesysbox_vectors.h"
#include "filesysbox_internal.h"

/****** filesysbox.library/FbxSetSignalCallback *****************************
*
*   NAME
*      FbxSetSignalCallback -- Set callback function for custom signals.
*                              (V53.23)
*
*   SYNOPSIS
*      void FbxSetSignalCallback(struct FbxFS * fs, 
*          FbxSignalCallbackFunc func, ULONG signals);
*
*   FUNCTION
*       Adds a callback function that will get called when the main filesystem
*       process receives one or more of the specified signals.
*
*   INPUTS
*       fs - The result of FbxSetupFS().
*       func - Callback function.
*       signals - Signals to call on.
*
*   RESULT
*       This function does not return a result
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

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
		struct Library *SysBase = fs->sysbase;

		ObtainSemaphore(&fs->fssema);

		fs->signalcallbackfunc = func;
		fs->signalcallbacksignals = signals;

		ReleaseSemaphore(&fs->fssema);
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

