/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

/****** filesysbox.library/FbxUninstallTimerCallback ************************
*
*   NAME
*      FbxUninstallTimerCallback -- Uninstall timer callback. (V53.23)
*
*   SYNOPSIS
*      void FbxUninstallTimerCallback(struct FbxFS * fs, 
*          struct FbxTimerCallbackData * cb);
*
*   FUNCTION
*       Use to remove a timer callback installed by FbxInstallTimerCallback().
*
*   INPUTS
*       fs - The result of FbxSetupFS().
*       cb - The result of FbxInstallTimerCallback().
*
*   RESULT
*       This function does not return a result
*
*   EXAMPLE
*
*   NOTES
*       Passing a NULL pointer as cb is safe and will do nothing.
*
*   BUGS
*
*   SEE ALSO
*       FbxInstallTimerCallback()
*
*****************************************************************************
*
*/

#ifdef __AROS__
AROS_LH2(void, FbxUninstallTimerCallback,
	AROS_LHA(struct FbxFS *, fs, A0),
	AROS_LHA(struct FbxTimerCallbackData *, cb, A1),
	struct FileSysBoxBase *, libBase, 14, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxUninstallTimerCallback(
	REG(a0, struct FbxFS *fs),
	REG(a1, struct FbxTimerCallbackData *cb),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	ADEBUGF("FbxUninstallTimerCallback(%#p, %#p)\n", fs, cb);

	if (fs != NULL && cb != NULL) {
		struct Library *SysBase = fs->sysbase;

		ObtainSemaphore(&fs->fssema);
		Remove((struct Node *)&cb->fschain);
		ReleaseSemaphore(&fs->fssema);

		FreeFbxTimerCallbackData(fs, cb);
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

