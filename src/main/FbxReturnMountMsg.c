/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

/****** filesysbox.library/FbxReturnMountMsg ********************************
*
*   NAME
*      FbxReturnMountMsg -- Return mount message
*
*   SYNOPSIS
*      void FbxReturnMountMsg(struct Message *msg, LONG r1, LONG r2);
*
*   FUNCTION
*       Returns mount message given to filesystem at startup.
*       Should never be called by filesystem unless there is a 
*       valid msg AND FbxSetupFS() was never called.
*
*   INPUTS
*       msg - mount message
*       r1 - DOSTRUE for success, DOSFALSE for failure.
*       r2 - error code if failure, else 0.
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
	struct Library   *DOSBase   = libBase->dosbase;

	ADEBUGF("FbxReturnMountMsg(%#p, %#p, %#p)\n", msg, (APTR)r1, (APTR)r2);

	if (msg != NULL)
	{
		struct DosPacket *pkt = (struct DosPacket *)msg->mn_Node.ln_Name;

		ReplyPkt(pkt, r1, r2);
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

