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
#include <string.h>

/****** filesysbox.library/FbxCleanupFS *************************************
*
*   NAME
*      FbxCleanupFS -- Delete a filesystem handle
*
*   SYNOPSIS
*      void FbxCleanupFS(struct FbxFS *fs);
*
*   FUNCTION
*       Cleans up any resources managed by the filesystem handle and
*       frees the handle itself.
*
*   INPUTS
*       fs - filesystem handle.
*
*   RESULT
*       This function does not return a result.
*
*   EXAMPLE
*
*   NOTES
*       Passing a NULL pointer as fs is safe and will do nothing.
*
*   BUGS
*
*   SEE ALSO
*       FbxSetupFS()
*
*****************************************************************************
*
*/

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
		if (fs->devnode != NULL) {
			fs->devnode->dn_Task = NULL;
			fs->devnode = NULL;
		}

		// doslist process
		if (fs->dlproc_port != NULL) {
			ObtainSemaphore(&libBase->procsema);
			if (--libBase->dlproc_refcount == 0)
				Signal(&libBase->dlproc->pr_Task, SIGBREAKF_CTRL_C);
			ReleaseSemaphore(&libBase->procsema);
		}

		// lockhandler process
		if (fs->lhproc_port != NULL) {
			ObtainSemaphore(&libBase->procsema);
			if (--libBase->lhproc_refcount == 0)
				Signal(&libBase->lhproc->pr_Task, SIGBREAKF_CTRL_C);
			ReleaseSemaphore(&libBase->procsema);
		}

		while ((chain = (struct MinNode *)RemHead((struct List *)&fs->timercallbacklist)) != NULL) {
			FreeFbxTimerCallbackData(fs, FSTIMERCALLBACKDATAFROMFSCHAIN(chain));
		}

		if (fs->fsflags & FBXF_ENABLE_DISK_CHANGE_DETECTION) {
			FbxRemDiskChangeHandler(fs);
		}

#ifdef ENABLE_CHARSET_CONVERSION
		if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
			if (fs->avlbuf != NULL)
				FreeMem(fs->avlbuf, 128*sizeof(struct FbxAVL));

			if (fs->maptable != NULL)
				FreeMem(fs->maptable, 256*sizeof(FbxUCS));
		}
#endif

		DeleteMsgPort(fs->fsport);
		DeleteMsgPort(fs->notifyreplyport);

		FreeSignal(fs->diskchangesig);
#ifndef NODEBUG
		FreeSignal(fs->dbgflagssig);
#endif

		DeletePool(fs->mempool);

		FbxCleanupTimerIO(fs);

		bzero(fs, sizeof(*fs));

		FreeFbxFS(fs);
	}

	ADEBUGF("FbxCleanupFS: DONE\n");

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

