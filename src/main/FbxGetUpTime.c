/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "filesysbox_vectors.h"
#include "filesysbox_internal.h"

/****** filesysbox.library/FbxGetUpTime ******************************************
*
*   NAME
*      FbxGetUpTime -- Get the current up time. (V54.5)
*
*   SYNOPSIS
*      void FbxGetUpTime(struct FbxFS *fs, struct timeval *tv);
*
*   FUNCTION
*       Reads the current up time and stores it in the supplied
*       timeval structure. Basically just a convenient way to call
*       GetUpTime() from a filesysbox handler that doesn't require
*       the "timer.device" to be opened again.
*
*   INPUTS
*       fs - The result of FbxSetupFS().
*       tv - TimeVal structure to store the time in.
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
AROS_LH2(void, FbxGetUpTime,
	AROS_LHA(struct FbxFS *, fs, A0),
	AROS_LHA(struct timeval *, tv, A1),
	struct FileSysBoxBase *, libBase, 20, FileSysBox)
{
	AROS_LIBFUNC_INIT

	struct Device *TimerBase = fs->timerbase;
	GetUpTime(tv);

	AROS_LIBFUNC_EXIT
}
#else
void FbxGetUpTime(
	REG(a0, struct FbxFS *fs),
	REG(a1, struct timeval *tv),
	REG(a6, struct FileSysBoxBase *libBase))
{
	FbxInternalGetUpTime(fs, tv);
}
#endif

