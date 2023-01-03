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
	ULONG freq;
	UQUAD eclock_current;
	UQUAD eclock_elapsed;

	freq = ReadEClock(&ev);
	eclock_current = ((UQUAD)ev.ev_hi << 32)|((UQUAD)ev.ev_lo);
	eclock_elapsed = eclock_current - fs->eclock_initial;

	tv->tv_secs = eclock_elapsed / freq;
	tv->tv_micro = (eclock_elapsed % freq) * 1000000ULL / freq;
}

