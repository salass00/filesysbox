/*
 * Dismount command for use with filesysbox clients.
 *
 * Copyright (c) 2013-2025 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "FbxDismount_rev.h"

#include <exec/alerts.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include <SDI/SDI_compiler.h>

#ifndef __AROS__
typedef ULONG IPTR;
typedef LONG SIPTR;
#endif

#define TEMPLATE "DEVICE/A"
enum {
	ARG_DEVICE,
	NUM_ARGS
};

static TEXT dosName[];
static TEXT template[];
static TEXT progName[];
static TEXT pktName[];

#ifdef __AROS__
AROS_UFH3(int, _start,
	AROS_UFHA(STRPTR, argstr, A0),
	AROS_UFHA(ULONG, arglen, D0),
	AROS_UFHA(struct Library *, SysBase, A6)
)
{
	AROS_USERFUNC_INIT
#else
int _start(void)
{
	struct Library *SysBase = *(struct Library **)4;
#endif
	struct Process *me = (struct Process *)FindTask(NULL);
	struct Library *DOSBase;
	struct RDArgs *rda;
	SIPTR args[NUM_ARGS];
	TEXT devname[256];
	struct DosPacket *pkt = NULL;
	struct DosList *dol;
	struct DeviceNode *dn;
	BOOL pktsent = FALSE;
	LONG error;
	int rc = RETURN_ERROR;

	DOSBase = OpenLibrary(dosName, 39);
	if (DOSBase == NULL)
	{
		Alert(AG_OpenLib | AO_DOSLib);
		return RETURN_FAIL;
	}

	bzero(args, sizeof(args));
	rda = ReadArgs(template, (APTR)args, NULL);
	if (rda == NULL)
	{
		PrintFault(error = IoErr(), progName);
		goto cleanup;
	}

	SplitName((CONST_STRPTR)args[ARG_DEVICE], ':', devname, 0, sizeof(devname));

	pkt = AllocDosObject(DOS_STDPKT, NULL);
	if (pkt == NULL)
	{
		PrintFault(error = ERROR_NO_FREE_STORE, progName);
		goto cleanup;
	}

	dol = LockDosList(LDF_DEVICES | LDF_READ);
	dn = (struct DeviceNode *)FindDosEntry(dol, devname, LDF_DEVICES | LDF_READ);
	if (dn != NULL && dn->dn_Task != NULL)
	{
		pkt->dp_Type = ACTION_DIE;
		SendPkt(pkt, dn->dn_Task, &me->pr_MsgPort);
		pktsent = TRUE;
	}
	UnLockDosList(LDF_DEVICES | LDF_READ);

	if (dn == NULL)
	{
		PrintFault(error = ERROR_OBJECT_NOT_FOUND, devname);
		goto cleanup;
	}

	if (pktsent)
	{
		WaitPkt();
		if (pkt->dp_Res1 == DOSFALSE)
		{
			PrintFault(error = pkt->dp_Res2, pktName);
			goto cleanup;
		}
	}

	dol = LockDosList(LDF_DEVICES | LDF_WRITE);
	dn = (struct DeviceNode *)FindDosEntry(dol, devname, LDF_DEVICES | LDF_WRITE);
	if (dn != NULL)
	{
		RemDosEntry((struct DosList *)dn);
	}
	UnLockDosList(LDF_DEVICES | LDF_WRITE);

	rc = RETURN_OK;

cleanup:
	if (pkt != NULL) FreeDosObject(DOS_STDPKT, pkt);
	if (rda != NULL) FreeArgs(rda);

	if (rc != RETURN_OK && error != 0)
	{
		SetIoErr(error);
	}

	CloseLibrary(DOSBase);

	return rc;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

static TEXT USED verstag[] = VERSTAG;
static TEXT dosName[] = "dos.library";
static TEXT template[] = TEMPLATE;
static TEXT progName[] = "FbxDismount";
static TEXT pktName[] = "ACTION_DIE";

