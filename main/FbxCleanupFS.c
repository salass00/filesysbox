/*
 * Copyright (c) 2008-2011 Leif Salomonsson
 * Copyright (c) 2013-2018 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

#ifdef __AROS__
AROS_LH1(void, FbxCleanupFS,
	AROS_LHA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 8, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxCleanupFS(
	REG(a0, struct FbxFS * fs),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	ADEBUGF("FbxCleanupFS(%#p)\n", fs);

	if (fs != NULL) {
		struct Library *SysBase = fs->sysbase;
		struct MinNode *chain;

		// clear msgport in device node.
		if (fs->devnode != NULL) fs->devnode->dn_Task = NULL;

		// doslist process
		if (fs->dlproc_port != NULL) {
			ObtainSemaphore(&libBase->dlproc_sem);
			if (--libBase->dlproc_refcount == 0)
				Signal(&libBase->dlproc->pr_Task, SIGBREAKF_CTRL_C);
			ReleaseSemaphore(&libBase->dlproc_sem);
		}

		while ((chain = (struct MinNode *)RemHead((struct List *)&fs->timercallbacklist)) != NULL) {
			FreeFbxTimerCallbackData(fs, FSTIMERCALLBACKDATAFROMFSCHAIN(chain));
		}

		if (fs->fsflags & FBXF_ENABLE_DISK_CHANGE_DETECTION) {
			FbxRemDiskChangeHandler(fs);
		}

		DeleteMsgPort(fs->fsport);
		DeleteMsgPort(fs->notifyreplyport);

		FreeSignal(fs->diskchangesig);
		FreeSignal(fs->dbgflagssig);

		DeletePool(fs->mempool);

		FbxCleanupTimerIO(fs);

		FreeFbxFS(fs);
	}

	ADEBUGF("FbxCleanupFS: DONE\n");

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

