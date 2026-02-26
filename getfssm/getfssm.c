/*
 * Dismount command for use with filesysbox clients.
 *
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <exec/alerts.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#ifndef ACTION_GET_DISK_FSSM
#define ACTION_GET_DISK_FSSM  4201
#define ACTION_FREE_DISK_FSSM 4202
#endif

#ifndef __AROS__
typedef ULONG IPTR;
typedef LONG SIPTR;
#endif

#define TEMPLATE "DEVICE/A"
enum {
	ARG_DEVICE,
	NUM_ARGS
};

#if !defined(__AROS__) || !defined(AROS_FAST_BSTR)
static void CopyStringBSTRToC(BSTR bstr, STRPTR cstr, ULONG size);
#endif

static const TEXT dosName[];
static const TEXT template[];
static const TEXT progName[];
static const TEXT unexpectedPktMsg[];

static const TEXT deviceInfoMsg[];
static const TEXT tableHeaderMsg[];
static const TEXT tableSizeMsg[];
static const TEXT secSizeMsg[];
static const TEXT surfacesMsg[];
static const TEXT secsPerBlkMsg[];
static const TEXT blksPerTrackMsg[];
static const TEXT reservedBlksMsg[];
static const TEXT lowCylMsg[];
static const TEXT highCylMsg[];
static const TEXT numBuffsMsg[];
static const TEXT memFlagsMsg[];
static const TEXT maxXferMsg[];
static const TEXT maskMsg[];
static const TEXT bootPriMsg[];
static const TEXT dosTypeMsg[];
static const TEXT baudRateMsg[];
static const TEXT ctrlMsg[];
static const TEXT bootBlksMsg[];
static const TEXT tableFooterMsg[];

#ifdef __AROS__
__startup AROS_UFH3(int, _start,
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
	struct DosPacket *pkt = NULL, *rp;
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
		pkt->dp_Type = ACTION_GET_DISK_FSSM;
		pkt->dp_Arg1 = 0;
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
		struct FileSysStartupMsg *fssm;
		struct DosEnvec *envtab;
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
		#define BTOC(bstr) ((CONST_STRPTR)(bstr))
#else
		TEXT buffer[256];
		#define BTOC(bstr) (CopyStringBSTRToC((bstr), buffer, sizeof(buffer)), buffer)
#endif

		while ((rp = WaitPkt()) != pkt)
		{
			Printf(unexpectedPktMsg, (IPTR)rp, rp->dp_Type);
		}
		fssm = (struct FileSysStartupMsg *)rp->dp_Res1;
		if (fssm == NULL)
		{
			PrintFault(error = rp->dp_Res2, devname);
			goto cleanup;
		}

		Printf(deviceInfoMsg, (IPTR)BTOC(fssm->fssm_Device), fssm->fssm_Unit, fssm->fssm_Flags);

		envtab = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
		PutStr(tableHeaderMsg);

		Printf(tableSizeMsg, envtab->de_TableSize);
		if (envtab->de_TableSize >= DE_SIZEBLOCK)
			Printf(secSizeMsg, envtab->de_SizeBlock<<2);
		if (envtab->de_TableSize >= DE_NUMHEADS)
			Printf(surfacesMsg, envtab->de_Surfaces);
#ifdef DE_SECSPERBLOCK
		if (envtab->de_TableSize >= DE_SECSPERBLOCK)
#else
		if (envtab->de_TableSize >= DE_SECSPERBLK)
#endif
			Printf(secsPerBlkMsg, envtab->de_SectorPerBlock);
		if (envtab->de_TableSize >= DE_BLKSPERTRACK)
			Printf(blksPerTrackMsg, envtab->de_BlocksPerTrack);
		if (envtab->de_TableSize >= DE_RESERVEDBLKS)
			Printf(reservedBlksMsg, envtab->de_Reserved);
		if (envtab->de_TableSize >= DE_LOWCYL)
			Printf(lowCylMsg, envtab->de_LowCyl);
		if (envtab->de_TableSize >= DE_UPPERCYL)
			Printf(highCylMsg, envtab->de_HighCyl);
		if (envtab->de_TableSize >= DE_NUMBUFFERS)
			Printf(numBuffsMsg, envtab->de_NumBuffers);
		if (envtab->de_TableSize >= DE_MEMBUFTYPE)
			Printf(memFlagsMsg, envtab->de_BufMemType);
		if (envtab->de_TableSize >= DE_MAXTRANSFER)
			Printf(maxXferMsg, envtab->de_MaxTransfer);
		if (envtab->de_TableSize >= DE_MASK)
			Printf(maskMsg, envtab->de_Mask);
		if (envtab->de_TableSize >= DE_BOOTPRI)
			Printf(bootPriMsg, envtab->de_BootPri);
		if (envtab->de_TableSize >= DE_DOSTYPE)
			Printf(dosTypeMsg, envtab->de_DosType);
		if (envtab->de_TableSize >= DE_BAUD)
			Printf(baudRateMsg, envtab->de_Baud);
		if (envtab->de_TableSize >= DE_CONTROL)
			Printf(ctrlMsg, (IPTR)BTOC(envtab->de_Control));
		if (envtab->de_TableSize >= DE_BOOTBLOCKS)
			Printf(bootBlksMsg, envtab->de_BootBlocks);

		PutStr(tableFooterMsg);

		pkt->dp_Type = ACTION_FREE_DISK_FSSM;
		pkt->dp_Arg1 = (SIPTR)fssm;
		SendPkt(pkt, dn->dn_Task, &me->pr_MsgPort);

		while ((rp = WaitPkt()) != pkt)
		{
			Printf(unexpectedPktMsg, (IPTR)rp, rp->dp_Type);
		}
		if (rp->dp_Res1 == DOSFALSE)
		{
			PrintFault(error = rp->dp_Res2, devname);
			goto cleanup;
		}
	}

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

static const TEXT dosName[] = "dos.library";
static const TEXT template[] = TEMPLATE;
static const TEXT progName[] = "FbxGetFSSM";
static const TEXT unexpectedPktMsg[] = "Received unexpected packet: 0x%lx (dp_Type: %ld)\n";

static const TEXT deviceInfoMsg[] =
	"Storage device driver\n"
	"=====================\n"
	"Device: %s\n"
	"Unit:   %lu\n"
	"Flags:  0x%08lx\n\n";

static const TEXT tableHeaderMsg[] =
	"Environment table\n"
	"=================\n";
static const TEXT tableSizeMsg[] =
	"Table size:        %lu longwords\n";
static const TEXT secSizeMsg[] =
	"Sector size:       %lu bytes\n";
static const TEXT surfacesMsg[] =
	"Surfaces:          %lu\n";
static const TEXT secsPerBlkMsg[] =
	"Sectors per block: %lu\n";
static const TEXT blksPerTrackMsg[] =
	"Blocks per track:  %lu\n";
static const TEXT reservedBlksMsg[] =
	"Reserved blocks:   %lu\n";
static const TEXT lowCylMsg[] =
	"First cylinder:    %lu\n";
static const TEXT highCylMsg[] =
	"Last cylinder:     %lu\n";
static const TEXT numBuffsMsg[] =
	"Number of buffers: %lu\n";
static const TEXT memFlagsMsg[] =
	"Memory flags:      0x%08lx\n";
static const TEXT maxXferMsg[] =
	"Max transfer:      0x%08lx\n";
static const TEXT maskMsg[] =
	"Mask:              0x%08lx\n";
static const TEXT bootPriMsg[] =
	"Boot priority:     %ld\n";
static const TEXT dosTypeMsg[] =
	"DOS Type:          0x%08lx\n";
static const TEXT baudRateMsg[] =
	"Baud rate:         %lu\n";
static const TEXT ctrlMsg[] =
	"Control:           %s\n";
static const TEXT bootBlksMsg[] =
	"Boot blocks:       %lu\n";
static const TEXT tableFooterMsg[] =
	"\n";

#if !defined(__AROS__) || !defined(AROS_FAST_BSTR)
static void CopyStringBSTRToC(BSTR bstr, STRPTR cstr, ULONG size) {
	struct Library *SysBase = *(struct Library **)4;
	UBYTE *src = BADDR(bstr);
	ULONG len = *src++;
	if (len >= size) len = size - 1;
	CopyMem(src, cstr, len);
	cstr[len] = '\0';
}
#endif

