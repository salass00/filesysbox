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
#include "filesysbox_vectors.h"
#include "filesysbox_internal.h"

/****** filesysbox.library/FbxQueryMountMsg *********************************
*
*   NAME
*      FbxQueryMountMsg -- Get information from mount message
*
*   SYNOPSIS
*      APTR FbxQueryMountMsg(struct Message *msg, LONG attr);
*
*   FUNCTION
*       Returns information from a mount message (normally
*       the first message passed to a filesystems port).
*
*       The following attributes can be queried:
*
*       FBXQMM_MOUNT_NAME (STRPTR)
*           Device name ("DH3", "USB1").
*
*       FBXQMM_MOUNT_CONTROL (STRPTR)
*           Control string if specified during mount.
*
*       FBXQMM_FSSM (struct FileSysStartupMsg *)
*           File system startup message.
*
*       FBXQMM_ENVIRON (struct DosEnvec *)
*           Disk environment structure.
*
*   INPUTS
*       msg - mount message
*       attr - attribute number
*
*   RESULT
*       The attributes value. If value is pointer to something,
*       do not assume it will be valid after mount message has
*       been returned.
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
AROS_LH2(APTR, FbxQueryMountMsg,
	AROS_LHA(struct Message *, msg, A0),
	AROS_LHA(LONG, attr, D0),
	struct FileSysBoxBase *, libBase, 5, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
APTR FbxQueryMountMsg(
	REG(a0, struct Message *msg),
	REG(d0, LONG attr),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	ADEBUGF("FbxQueryMountMsg(%#p, %ld)\n", msg, attr);

	struct DosPacket *pkt = (struct DosPacket *)msg->mn_Node.ln_Name;
	struct DeviceNode *devnode = BADDR(pkt->dp_Arg3);
	struct FileSysStartupMsg *fssm;
	struct DosEnvec *de;

	switch (attr) {
	case FBXQMM_MOUNT_NAME:
#ifdef __AROS__
		return AROS_BSTR_ADDR(devnode->dn_Name);
#else
		return BADDR(devnode->dn_Name) + 1;
#endif
	case FBXQMM_MOUNT_CONTROL:
		fssm = FbxGetFSSM(libBase->sysbase, devnode);
		if (fssm != NULL) {
			de = BADDR(fssm->fssm_Environ);
			if (IS_VALID_BPTR(de->de_Control) && de->de_Control > 4095)
#ifdef __AROS__
				return AROS_BSTR_ADDR(de->de_Control);
#else
				return BADDR(de->de_Control) + 1;
#endif
		}
		return NULL;
	case FBXQMM_FSSM:
		fssm = FbxGetFSSM(libBase->sysbase, devnode);
		return fssm;
	case FBXQMM_ENVIRON:
		fssm = FbxGetFSSM(libBase->sysbase, devnode);
		if (fssm != NULL) {
			return BADDR(fssm->fssm_Environ);
		}
	default:
		return NULL;
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

