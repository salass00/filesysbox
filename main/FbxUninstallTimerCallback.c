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

		Remove((struct Node *)&cb->fschain);
		FreeFbxTimerCallbackData(fs, cb);
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

