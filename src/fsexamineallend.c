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

extern struct Library *SysBase;

void FreeFbxDirData(APTR pool, struct FbxDirData *dd) {
	DEBUGF("FreeFbxDirData(%#p)\n", dd);

	if (dd != NULL) {
		if (dd->name != NULL)
			FreeVecPooled(pool, dd->name);

		if (dd->comment != NULL)
			FreeVecPooled(pool, dd->comment);

		FreeVecPooled(pool, dd);
	}
}

void FreeFbxDirDataList(APTR pool, struct MinList *list) {
	struct MinNode *chain, *succ;

	DEBUGF("FreeFbxDirDataList(%#p)\n", list);

	chain = list->mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		FreeFbxDirData(pool, FSDIRDATAFROMNODE(chain));
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
				FreeFbxDirDataList(fs->mempool, &exallstate->freelist);
				FreeFbxExAllState(fs, exallstate);
			}
			ctrl->eac_LastKey = (IPTR)NULL;
		}
	}

	if (lock != NULL) FreeFbxDirDataList(fs->mempool, &lock->dirdatalist);

	fs->r2 = 0;
	return DOSTRUE;
}

