/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

struct timerequest *FbxSetupTimerIO(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct MsgPort *mp;
	struct timerequest *tr;

	DEBUGF("FbxSetupTimerIO(%#p)\n", fs);

	mp = CreateMsgPort();
	tr = CreateIORequest(mp, sizeof(*tr));
	if (tr == NULL) {
		DeleteMsgPort(mp);
		return NULL;
	}

	if (OpenDevice((CONST_STRPTR)"timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0) != 0) {
		DeleteIORequest(tr);
		DeleteMsgPort(mp);
		return NULL;
	}

	fs->timerio   = tr;
	fs->timerbase = tr->tr_node.io_Device;
	fs->timerbusy = FALSE;

	return tr;
}

void FbxCleanupTimerIO(struct FbxFS *fs) {
	DEBUGF("FbxCleanupTimerIO(%#p)\n", fs);

	if (fs->timerbase != NULL) {
		struct Library *SysBase = fs->sysbase;
		struct timerequest *tr = fs->timerio;
		struct MsgPort *mp = tr->tr_node.io_Message.mn_ReplyPort;

		CloseDevice((struct IORequest *)tr);
		DeleteIORequest(tr);
		DeleteMsgPort(mp);
	}
}

void FbxInitUpTime(struct FbxFS *fs) {
	struct Device *TimerBase = fs->timerbase;
	struct EClockVal ev;

	ReadEClock(&ev);
	fs->eclock_initial = ((UQUAD)ev.ev_hi << 32)|((UQUAD)ev.ev_lo);
}

void FbxGetUpTime(struct FbxFS *fs, struct timeval *tv) {
	struct Device *TimerBase = fs->timerbase;
	struct EClockVal ev;
	UQUAD eclock_current;
	UQUAD eclock_elapsed;
	ULONG freq, seconds, remainder, micros;

	freq = ReadEClock(&ev);
	eclock_current = ((UQUAD)ev.ev_hi << 32)|((UQUAD)ev.ev_lo);
	eclock_elapsed = eclock_current - fs->eclock_initial;

#ifdef __mc68020
	ULONG dummy;
	__asm__("divul %4,%0:%1"
		: "=d" (remainder), "=d" (seconds)
		: "0" ((ULONG)(eclock_elapsed >> 32)), "1" ((ULONG)eclock_elapsed), "d" (freq)
		: "cc"
	);
	__asm__("mulul #1000000,%1:%0\n"
		"\tdivul %3,%1:%0"
		: "=&d" (micros), "=&d" (dummy)
		: "0" (remainder), "d" (freq)
		: "cc"
	);
#else
	seconds = eclock_elapsed / freq;
	remainder = eclock_elapsed % freq;
	micros = ((UQUAD)remainder * 1000000ULL) / freq;
#endif
	tv->tv_secs = seconds;
	tv->tv_micro = micros;
}

QUAD FbxGetUpTimeMillis(struct FbxFS *fs) {
	struct timeval tv;

	FbxGetUpTime(fs, &tv);
	return (UQUAD)tv.tv_secs * 1000 + tv.tv_micro / 1000;
}

