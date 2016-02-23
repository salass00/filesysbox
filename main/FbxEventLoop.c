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
#include <string.h>

void FbxReadDebugFlags(struct FbxFS *fs);

#ifdef __AROS__
AROS_LH1(LONG, FbxEventLoop,
	AROS_LHA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 7, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
LONG FbxEventLoop(
	REG(a0, struct FbxFS *fs),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct Library *SysBase = fs->sysbase;
	struct Library *DOSBase = fs->dosbase;
	struct NotifyRequest nr;
	LONG run = TRUE;

	ADEBUGF("FbxEventLoop(%#p)\n", fs);

	fs->dosetup = TRUE;

	const ULONG packsig       = 1UL << fs->fsport->mp_SigBit;
	const ULONG notrepsig     = 1UL << fs->notifyreplyport->mp_SigBit;
	const ULONG timesig       = 1UL << fs->timerio->tr_node.io_Message.mn_ReplyPort->mp_SigBit;
	const ULONG dbgflagssig   = 1UL << fs->dbgflagssig;
	const ULONG diskchangesig = 1UL << fs->diskchangesig;
	const ULONG wsigs         = packsig | notrepsig | timesig | dbgflagssig | diskchangesig;

	FbxStartTimer(fs);

	bzero(&nr, sizeof(nr));
	nr.nr_Name = (STRPTR)"ENV:FBX_DBGFLAGS";
	nr.nr_Flags = NRF_SEND_SIGNAL;
	nr.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
	nr.nr_stuff.nr_Signal.nr_SignalNum = fs->dbgflagssig;

	StartNotify(&nr);

	while (run) {
		if (fs->dosetup) {
			FbxCleanupVolume(fs);
			fs->dosetup = FALSE;
			FbxSetupVolume(fs);
		}

		const ULONG usigs = fs->signalcallbacksignals;
		const ULONG rsigs = Wait(wsigs | usigs);

		if (rsigs & dbgflagssig) FbxReadDebugFlags(fs);
		if (rsigs & packsig) FbxHandlePackets(fs);
		if (rsigs & notrepsig) FbxHandleNotifyReplies(fs);
		if (rsigs & timesig) FbxHandleTimerEvent(fs);
		if (rsigs & usigs) FbxHandleUserEvent(fs, rsigs);
		if (fs->shutdown) run = FALSE;
	}

	EndNotify(&nr);

	FbxStopTimer(fs);

	return 0;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

