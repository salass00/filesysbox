/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <clib/debug_protos.h>

#if !defined(__AROS__) && !defined(NODEBUG)
void KPutStr(CONST_STRPTR str) {
	__asm__ __volatile__
	(
		"move.l 4.w,a6\n\t"
		"bra.s 2f\n"
		"1:\n\t"
		"jsr -516(a6)\n"
		"2:\n\t"
		"move.b (%0)+,d0\n\t"
		"bne.s 1b"
		:
		: "a" (str)
		: "d0", "a6", "cc"
	);
}
#endif

