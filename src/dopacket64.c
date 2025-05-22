/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifdef ENABLE_DP64_SUPPORT
#include "filesysbox_internal.h"

QUAD FbxDoPacket64(struct FbxFS *fs, struct DosPacket64 *pkt) {
	LONG type;
	QUAD r1;

	PDEBUGF("FbxDoPacket64(%#p, %#p)\n", fs, pkt);

#ifndef NODEBUG
	struct Task *callertask = pkt->dp_Port->mp_SigTask;
	PDEBUGF("action %ld task %#p '%s'\n", pkt->dp_Type, callertask, callertask->tc_Node.ln_Name);
#endif

	type = pkt->dp_Type;
	switch (type) {
	case ACTION_GET_FILE_POSITION64:
		r1 = FbxGetFilePosition(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_GET_FILE_SIZE64:
		r1 = FbxGetFileSize(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	default:
		r1 = DOSFALSE;
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		break;
	}

	PDEBUGF("Done with packet %#p. r1 %lld r2 %#p\n\n", pkt, r1, (APTR)fs->r2);

	return r1;
}

#endif /* ENABLE_DP64_SUPPORT */

