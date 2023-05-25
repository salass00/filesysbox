/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

void FreeFbxDirData(struct FbxFS *fs, struct FbxDirData *dd) {
#ifdef __AROS__
	struct Library *SysBase = fs->sysbase;
#endif

	DEBUGF("FreeFbxDirData(%#p, %#p)\n", fs, dd);

	if (dd != NULL) {
		if (dd->name != NULL)
			FreeVecPooled(fs->mempool, dd->name);

		if (dd->comment != NULL)
			FreeVecPooled(fs->mempool, dd->comment);

		FreeVecPooled(fs->mempool, dd);
	}
}

void FreeFbxDirDataList(struct FbxFS *fs, struct MinList *list) {
	struct MinNode *chain, *succ;

	DEBUGF("FreeFbxDirDataList(%#p, %#p)\n", fs, list);

	chain = list->mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		FreeFbxDirData(fs, FSDIRDATAFROMNODE(chain));
		chain = succ;
	}

	NEWMINLIST(list);
}

int FbxExamineAllEnd(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, SIPTR len,
	int type, struct ExAllControl *ctrl)
{
	struct Library *SysBase = fs->sysbase;
	struct FbxExAllState *exallstate;

	PDEBUGF("FbxExamineAllEnd(%#p, %#p, %#p, %d, %d, %#p)\n", fs, lock, buffer, len, type, ctrl);

	if (ctrl != NULL) {
		exallstate = (struct FbxExAllState *)ctrl->eac_LastKey;
		if (exallstate) {
			if (exallstate != (APTR)-1) {
				FreeFbxDirDataList(fs, &exallstate->freelist);
				FreeFbxExAllState(fs, exallstate);
			}
			ctrl->eac_LastKey = (IPTR)NULL;
		}
	}

	if (lock != NULL) FreeFbxDirDataList(fs, &lock->dirdatalist);

	fs->r2 = 0;
	return DOSTRUE;
}

