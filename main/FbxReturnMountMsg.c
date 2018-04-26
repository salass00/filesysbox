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
AROS_LH3(void, FbxReturnMountMsg,
	AROS_LHA(struct Message *, msg, A0),
	AROS_LHA(SIPTR, r1, D0),
	AROS_LHA(SIPTR, r2, D1),
	struct FileSysBoxBase *, libBase, 9, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxReturnMountMsg(
	REG(a0, struct Message *msg),
	REG(d0, SIPTR r1),
	REG(d1, SIPTR r2),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct Library   *SysBase   = libBase->sysbase;
	struct DosPacket *pkt       = (struct DosPacket *)msg->mn_Node.ln_Name;
	struct MsgPort   *replyport = pkt->dp_Port;

	ADEBUGF("FbxReturnMountMsg(%#p, %#p, %#p)\n", msg, (APTR)r1, (APTR)r2);

	pkt->dp_Res1 = r1;
	pkt->dp_Res2 = r2;

	PutMsg(replyport, msg);

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

