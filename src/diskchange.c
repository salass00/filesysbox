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
#include <devices/trackdisk.h>
#include <SDI/SDI_interrupt.h>
#include <string.h>

#ifdef __AROS__
AROS_UFH5(int, FbxDiskChangeInterrupt,
	AROS_UFHA(APTR, data, A1),
	AROS_UFHA(APTR, code, A5),
	AROS_UFHA(struct ExecBase *, SysBase, A6),
	AROS_UFHA(APTR, mask, D1),
	AROS_UFHA(APTR, custom, A0))
{
	AROS_USERFUNC_INIT
#else
INTERRUPTPROTO(FbxDiskChangeInterrupt, int, APTR custom, APTR data) {
#endif
	struct FbxFS *fs = data;
	struct FbxDiskChangeHandler *dch = fs->diskchangehandler;

	if (dch != NULL) {
		dch->func(fs);
	}

	return 0;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

struct FbxDiskChangeHandler *FbxAddDiskChangeHandler(struct FbxFS *fs, FbxDiskChangeHandlerFunc func) {
	struct Library *SysBase = fs->sysbase;
	struct FbxDiskChangeHandler *dch;
	struct FileSysStartupMsg *fssm = fs->fssm;
	struct MsgPort *mp = NULL;
	struct IOStdReq *io = NULL;
	struct Interrupt *interrupt = NULL;
	char devname[256];

	dch = AllocPooled(fs->mempool, sizeof(*dch));
	if (dch == NULL) goto cleanup;

	dch->func = func;

	mp = CreateMsgPort();
	io = (struct IOStdReq *)CreateIORequest(mp, sizeof(*io));
	if (io == NULL) goto cleanup;

	CopyStringBSTRToC(fssm->fssm_Device, devname, sizeof(devname));
	if (OpenDevice((CONST_STRPTR)devname, fssm->fssm_Unit, (struct IORequest *)io, fssm->fssm_Flags) != 0) {
		io->io_Device = NULL;
		goto cleanup;
	}

	interrupt = AllocPooled(fs->mempool, sizeof(*interrupt));
	if (interrupt == NULL) goto cleanup;

	bzero(interrupt, sizeof(*interrupt));
	interrupt->is_Node.ln_Type = NT_INTERRUPT;
#ifdef __AROS__
	interrupt->is_Node.ln_Name = (char *)AROS_BSTR_ADDR(fs->devnode->dn_Name);
	interrupt->is_Code         = (void (*)())FbxDiskChangeInterrupt;
#else
	interrupt->is_Node.ln_Name = (char *)BADDR(fs->devnode->dn_Name) + 1;
	interrupt->is_Code         = (void (*)())ENTRY(FbxDiskChangeInterrupt);
#endif
	interrupt->is_Data = fs;

	io->io_Command = TD_ADDCHANGEINT;
	io->io_Data    = interrupt;
	io->io_Length  = sizeof(*interrupt);
	SendIO((struct IORequest *)io);

	dch->io = io;

	fs->diskchangehandler = dch;
	return dch;

cleanup:
	FreePooled(fs->mempool, interrupt, sizeof(*interrupt));
	if (io != NULL && io->io_Device != NULL) {
		CloseDevice((struct IORequest *)io);
	}
	DeleteIORequest((struct IORequest *)io);
	DeleteMsgPort(mp);
	FreePooled(fs->mempool, dch, sizeof(*dch));

	return NULL;
}

void FbxRemDiskChangeHandler(struct FbxFS *fs) {
	struct FbxDiskChangeHandler *dch = fs->diskchangehandler;

	if (dch != NULL) {
		struct Library *SysBase = fs->sysbase;
		struct IOStdReq *io = dch->io;
		struct MsgPort *mp = io->io_Message.mn_ReplyPort;
		struct Interrupt *interrupt = (struct Interrupt *)io->io_Data;

		if (CheckIO((struct IORequest *)io) == NULL) {
			io->io_Command = TD_REMCHANGEINT;
			DoIO((struct IORequest *)io);
		} else {
			WaitIO((struct IORequest *)io);
		}

		CloseDevice((struct IORequest *)io);
		DeleteIORequest((struct IORequest *)io);
		DeleteMsgPort(mp);
		FreePooled(fs->mempool, interrupt, sizeof(*interrupt));
		FreePooled(fs->mempool, dch, sizeof(*dch));
	}
}

