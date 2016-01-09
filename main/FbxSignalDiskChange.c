/*
 * Copyright (c) 2008-2011 Leif Salomonsson
 * Copyright (c) 2013-2016 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

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

