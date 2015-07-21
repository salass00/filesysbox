/*
 * Copyright (c) 2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

void FbxInitUpTime(struct FbxFS *fs) {
	struct EClockVal ev;

	GetTimerBase

	ReadEClock(&ev);
	fs->eclock_initial = ((UQUAD)ev.ev_hi << 32)|((UQUAD)ev.ev_lo);
}

void FbxGetUpTime(struct FbxFS *fs, struct timeval *tv) {
	struct EClockVal ev;
	ULONG freq;
	UQUAD eclock_current;
	UQUAD eclock_elapsed;

	GetTimerBase

	freq = ReadEClock(&ev);
	eclock_current = ((UQUAD)ev.ev_hi << 32)|((UQUAD)ev.ev_lo);
	eclock_elapsed = eclock_current - fs->eclock_initial;

	tv->tv_secs = eclock_elapsed / freq;
	tv->tv_micro = (eclock_elapsed % freq) * 1000000ULL / freq;
}

