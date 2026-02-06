/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

extern struct Library *SysBase;

void FreeFbxDirData(struct FbxLock *lock, struct FbxDirData *dd) {
	if (dd != NULL) {
		APTR pool = lock->mempool;

		if (dd->name != NULL)
			FreeVecPooled(pool, dd->name);

		if (dd->comment != NULL)
			FreeVecPooled(pool, dd->comment);

		FreeVecPooled(pool, dd);
	}
}

void FreeFbxDirDataList(struct FbxLock *lock, struct MinList *list) {
	struct MinNode *chain, *succ;

	for (chain = list->mlh_Head; (succ = chain->mln_Succ) != NULL; chain = succ) {
		FreeFbxDirData(lock, FSDIRDATAFROMNODE(chain));
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
				FreeFbxDirDataList(lock, &exallstate->freelist);
				FreeFbxExAllState(lock, exallstate);
			}
			ctrl->eac_LastKey = (IPTR)NULL;
		}
	}

	if (lock != NULL) FreeFbxDirDataList(lock, &lock->dirdatalist);

	fs->r2 = 0;
	return DOSTRUE;
}

