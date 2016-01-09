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
		fssm = BADDR(devnode->dn_Startup);
		de = BADDR(fssm->fssm_Environ);
#ifdef __AROS__
		return (de->de_Control > 4095) ? AROS_BSTR_ADDR(de->de_Control) : NULL;
#else
		return (de->de_Control > 4095) ? (BADDR(de->de_Control) + 1) : NULL;
#endif
	case FBXQMM_FSSM:
		fssm = BADDR(devnode->dn_Startup);
		return fssm;
	case FBXQMM_ENVIRON:
		fssm = BADDR(devnode->dn_Startup);
		de = BADDR(fssm->fssm_Environ);
		return de;
	default:
		return NULL;
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

