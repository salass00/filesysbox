/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "filesysbox_vectors.h"
#include "filesysbox_internal.h"
#include <string.h>

static void FbxStartTimer(struct FbxFS *fs);
static void FbxStopTimer(struct FbxFS *fs);
static void FbxHandlePackets(struct FbxFS *fs);
static void FbxHandleNotifyReplies(struct FbxFS *fs);
static void FbxHandleTimerEvent(struct FbxFS *fs);
static void FbxHandleUserEvent(struct FbxFS *fs, ULONG signals);

/****** filesysbox.library/FbxEventLoop *************************************
*
*   NAME
*      FbxEventLoop -- Enter filesystem event loop.
*
*   SYNOPSIS
*      LONG FbxEventLoop(struct FbxFS *fs);
*
*   FUNCTION
*       Starts up the filesystem and makes it visible to the 
*       operating system. Handles incoming packets and calls 
*       the appropriate methods in the fuse_operations table. 
*
*   INPUTS
*       fs - The result of FbxSetupFS().
*
*   RESULT
*       0 for success. no errorcodes are defined for now.
*
*   EXAMPLE
*
*   NOTES
*       When this function returns, the handler should cleanup any 
*       resources and exit.
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

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
#ifndef NODEBUG
	struct Library *DOSBase = fs->dosbase;
	struct NotifyRequest nr;
#endif
	LONG run = TRUE;

	ADEBUGF("FbxEventLoop(%#p)\n", fs);

	fs->dosetup = TRUE;

	const ULONG packsig       = 1UL << fs->fsport->mp_SigBit;
	const ULONG notrepsig     = 1UL << fs->notifyreplyport->mp_SigBit;
	const ULONG timesig       = 1UL << fs->timerio->tr_node.io_Message.mn_ReplyPort->mp_SigBit;
#ifndef NODEBUG
	const ULONG dbgflagssig   = 1UL << fs->dbgflagssig;
#else
	const ULONG dbgflagssig   = 0;
#endif
	const ULONG diskchangesig = 1UL << fs->diskchangesig;
	const ULONG wsigs         = packsig | notrepsig | timesig | dbgflagssig | diskchangesig;

	FbxStartTimer(fs);

#ifndef NODEBUG
	memset(&nr, 0, sizeof(nr));
	nr.nr_Name = (STRPTR)"ENV:FBX_DBGFLAGS";
	nr.nr_Flags = NRF_SEND_SIGNAL;
	nr.nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
	nr.nr_stuff.nr_Signal.nr_SignalNum = fs->dbgflagssig;

	StartNotify(&nr);
#endif

	while (run) {
		if (fs->dosetup) {
			ObtainSemaphore(&fs->fssema);
			FbxCleanupVolume(fs);
			fs->dosetup = FALSE;
			FbxSetupVolume(fs);
			ReleaseSemaphore(&fs->fssema);
		}

		const ULONG usigs = fs->signalcallbacksignals;
		const ULONG rsigs = Wait(wsigs | usigs);

#ifndef NODEBUG
		if (rsigs & dbgflagssig) FbxReadDebugFlags(fs);
#endif
		if (rsigs & packsig) FbxHandlePackets(fs);
		if (rsigs & notrepsig) FbxHandleNotifyReplies(fs);
		if (rsigs & timesig) FbxHandleTimerEvent(fs);
		if (rsigs & usigs) FbxHandleUserEvent(fs, rsigs);
		if (fs->shutdown) run = FALSE;
	}

#ifndef NODEBUG
	EndNotify(&nr);
#endif

	FbxStopTimer(fs);

	return 0;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

static void FbxStartTimer(struct FbxFS *fs) {
	if (!fs->timerbusy) {
		struct Library *SysBase = fs->sysbase;
		struct timerequest *tr = fs->timerio;

		tr->tr_node.io_Command = TR_ADDREQUEST;
		tr->tr_time.tv_secs = 0;
		tr->tr_time.tv_micro = FBX_TIMER_MICROS;
		SendIO((struct IORequest *)tr);
		fs->timerbusy = TRUE;
	}
}

static void FbxStopTimer(struct FbxFS *fs) {
	if (fs->timerbusy) {
		struct Library *SysBase = fs->sysbase;
		struct timerequest *tr = fs->timerio;

		AbortIO((struct IORequest *)tr);
		WaitIO((struct IORequest *)tr);
		fs->timerbusy = FALSE;
	}
}

static void FbxReturnPacket(struct FbxFS *fs, struct DosPacket *pkt, SIPTR r1, SIPTR r2) {
	struct Library *DOSBase = fs->dosbase;

	pkt->dp_Res1 = r1;
	pkt->dp_Res2 = r2;

	SendPkt(pkt, pkt->dp_Port, fs->fsport);
}

#ifdef ENABLE_DP64_SUPPORT
static void FbxReturnPacket64(struct FbxFS *fs, struct DosPacket64 *pkt, QUAD r1, SIPTR r2) {
	struct Library *DOSBase = fs->dosbase;

	pkt->dp_Res1 = r1;
	pkt->dp_Res2 = r2;

	SendPkt((struct DosPacket *)pkt, pkt->dp_Port, fs->fsport);
}
#endif /* ENABLE_DP64_SUPPORT */

static void FbxHandlePackets(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct Message *msg;
	struct DosPacket *pkt;

	DEBUGF("FbxHandlePackets(%#p)\n", fs);

	while ((msg = GetMsg(fs->fsport)) != NULL) {
		pkt = (struct DosPacket *)msg->mn_Node.ln_Name;
#ifdef ENABLE_DP64_SUPPORT
		if (pkt->dp_Type > 8000 && pkt->dp_Type < 9000)
		{
			struct DosPacket64 *pkt64 = (struct DosPacket64 *)pkt;
			QUAD r1 = FbxDoPacket64(fs, pkt64);
			FbxReturnPacket64(fs, pkt64, r1, fs->r2);
		}
		else
#endif /* ENABLE_DP64_SUPPORT */
		{
			SIPTR r1 = FbxDoPacket(fs, pkt);
			FbxReturnPacket(fs, pkt, r1, fs->r2);
		}
	}
}

static void FbxHandleNotifyReplies(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct NotifyMessage *nm;
	struct NotifyRequest *nr;

	NDEBUGF("FbxHandleNotifyReplies(%#p)\n", fs);

	while ((nm = (struct NotifyMessage *)GetMsg(fs->notifyreplyport)) != NULL) {
		nr = nm->nm_NReq;
		if (nr->nr_Flags & NRF_MAGIC) {
			// reuse request and send it one more time
			nr->nr_Flags &= ~NRF_MAGIC;
			PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *)nm);
		} else {
			nr->nr_MsgCount--;
			FreeNotifyMessage(nm);
		}
	}
}

static void FbxHandleTimerEvent(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct Message *msg;

	//DEBUGF("FbxHandleTimerEvent(%#p)\n", fs);

	msg = GetMsg(fs->timerio->tr_node.io_Message.mn_ReplyPort);
	if (msg != NULL) {
		fs->timerbusy = FALSE;

		if (fs->localebase != NULL) {
			struct Library *LocaleBase = fs->localebase;
			struct Locale *locale;
			// TODO cache the locale
			if ((locale = OpenLocale(NULL))) {
				fs->gmtoffset = (int)locale->loc_GMTOffset;
				CloseLocale(locale);
			}
		}

		if (!IsMinListEmpty(&fs->timercallbacklist)) {
			struct MinNode *chain, *succ;

			ObtainSemaphore(&fs->fssema);

			chain = fs->timercallbacklist.mlh_Head;
			while ((succ = chain->mln_Succ) != NULL) {
				struct FbxTimerCallbackData *cb = FSTIMERCALLBACKDATAFROMFSCHAIN(chain);
				ULONG currtime = FbxGetUpTimeMillis(fs);
				LONG x = (LONG)(currtime - cb->lastcall);
				if (cb->period != 0 && x > cb->period) {
					cb->lastcall = currtime;
					cb->func();
				}
				chain = succ;
			}

			ReleaseSemaphore(&fs->fssema);
		}

		if (fs->aut != 0 || fs->iaut != 0) {
			ObtainSemaphore(&fs->fssema);

			if (OKVOLUME(fs->currvol) && fs->firstmodify) {
				ULONG currtime = FbxGetUpTimeMillis(fs);
				LONG x = (LONG)(currtime - fs->firstmodify);
				LONG y = (LONG)(currtime - fs->lastmodify);
				if (fs->aut != 0 && x > fs->aut) {
					FbxFlushAll(fs);
				} else if (fs->iaut != 0 && y > fs->iaut) {
					FbxFlushAll(fs);
				}
			}

			ReleaseSemaphore(&fs->fssema);
		}

		FbxStartTimer(fs);
	}
}

static void FbxHandleUserEvent(struct FbxFS *fs, ULONG signals) {
	struct Library *SysBase = fs->sysbase;

	//DEBUGF("FbxHandleUserEvent(%#p, %#lx)\n", fs, signals);

	ObtainSemaphore(&fs->fssema);

	signals &= fs->signalcallbacksignals;
	if (signals != 0 && fs->signalcallbackfunc != NULL) {
		fs->signalcallbackfunc(signals);
	}

	ReleaseSemaphore(&fs->fssema);
}

