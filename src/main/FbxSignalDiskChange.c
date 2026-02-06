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

/****** filesysbox.library/FbxSignalDiskChange ******************************
*
*   NAME
*      FbxSignalDiskChange -- Signal diskchange to filesystem process. (V53.35)
*
*   SYNOPSIS
*      void FbxSignalDiskChange(struct FbxFS * fs);
*
*   FUNCTION
*       Only needed if the standard trackdisk.device method of disk change
*       detection (through FBXF_ENABLE_DISK_CHANGE_DETECTION) does not suit
*       your filesystem for some reason.
*
*   INPUTS
*       fs - filesystem handle.
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
AROS_LH1(void, FbxSignalDiskChange,
	AROS_LHA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 15, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxSignalDiskChange(
	REG(a0, struct FbxFS *fs),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct Library *SysBase = fs->sysbase;

	ADEBUGF("FbxSignalDiskChange(%#p)\n", fs);

	fs->dosetup = TRUE;
	Signal(&fs->thisproc->pr_Task, 1UL << fs->diskchangesig);

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

