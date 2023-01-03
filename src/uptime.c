/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

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
	);
	__asm__("mulul #1000000,%1:%0\n"
		"\tdivul %3,%1:%0"
		: "=&d" (micros), "=&d" (dummy)
		: "0" (remainder), "d" (freq)
	);
#else
	seconds = eclock_elapsed / freq;
	remainder = eclock_elapsed % freq;
	micros = ((UQUAD)remainder * 1000000ULL) / freq;
#endif
	tv->tv_secs = seconds;
	tv->tv_micro = micros;
}

