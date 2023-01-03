/*
 * UTF-8 support routines
 *
 * Copyright (c) 2013-2018 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 *
 * The utf8_decode_slow() and utf8_decode_fast() implementations are based on:
 * Basic UTF-8 manipulation routines by Jeff Bezanson.
 * Placed in the public domain Autumn 2005.
 */

#include "filesysbox_internal.h"

static const UBYTE utf8_trailing_bytes[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const ULONG utf8_offsets[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const UBYTE utf8_lb_masks[6] = {
	0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01
};

static const ULONG utf8_first_codes[6] = {
	0x0, 0x80, 0x800, 0x10000, 0x200000, 0x4000000
};

// slow, validating decode function
LONG utf8_decode_slow(const char **strp) {
	const UBYTE *str = (const UBYTE *)*strp;
	UBYTE byte, ntb;
	ULONG unicode;
	if (((byte = *str++) & 0xC0) == 0x80) return -1;
	ntb = utf8_trailing_bytes[byte];
	if (ntb >= 4) return -1;
	if (ntb == 0) {
		*strp = (const char *)str;
		return byte;
	}
	unicode = byte & utf8_lb_masks[ntb];
	switch (ntb) {
	case 3:
		if (((byte = *str++) & 0xC0) != 0x80) return -1;
		unicode <<= 6;
		unicode |= byte & 0x3F;
	case 2:
		if (((byte = *str++) & 0xC0) != 0x80) return -1;
		unicode <<= 6;
		unicode |= byte & 0x3F;
	case 1:
		if (((byte = *str++) & 0xC0) != 0x80) return -1;
		unicode <<= 6;
		unicode |= byte & 0x3F;
	}
	// check for invalid and overlong codes
	if (unicode < utf8_first_codes[ntb] ||
		(unicode >= 0xD800 && unicode <= 0xDFFF) ||
		unicode == 0xFFFE || unicode == 0xFFFF ||
		unicode > 0x10FFFF)
	{
		return -1;
	}
	*strp = (const char *)str;
	return unicode;
}

// faster decode function which assumes valid UTF-8
inline LONG utf8_decode_fast(const char **strp) {
	const UBYTE *str = (const UBYTE *)*strp;
	UBYTE ntb = utf8_trailing_bytes[*str];
	ULONG unicode = 0;
	switch (ntb) {
	case 3: unicode += *str++; unicode <<= 6;
	case 2: unicode += *str++; unicode <<= 6;
	case 1: unicode += *str++; unicode <<= 6;
	case 0: unicode += *str++;
	}
	unicode -= utf8_offsets[ntb];
	*strp = (const char *)str;
	return unicode;
}

size_t utf8_strlen(const char *str) {
	size_t len = 0;
	int byte;
	while ((byte = (UBYTE)*str) != '\0') {
		str += 1 + utf8_trailing_bytes[byte];
		len++;
	}
	return len;
}

int utf8_stricmp(const char *s1, const char *s2) {
	LONG c1, c2;
	do {
		c1 = ucs4_toupper(utf8_decode_fast(&s1));
		c2 = ucs4_toupper(utf8_decode_fast(&s2));
	} while (c1 != '\0' && c1 == c2);
	return c1 - c2;
}

int utf8_strnicmp(const char *s1, const char *s2, size_t n) {
	LONG c1, c2;
	if (n == 0) return 0;
	do {
		c1 = ucs4_toupper(utf8_decode_fast(&s1));
		c2 = ucs4_toupper(utf8_decode_fast(&s2));
	} while (c1 != '\0' && --n > 0 && c1 == c2);
	return c1 - c2;
}

size_t utf8_strlcpy(char *dst, const char *src, size_t dst_size) {
	char *dst_start = dst;
	char *dst_end = dst + dst_size;
	int byte, utf8_size;
	if (dst_end > dst) {
		while ((byte = (UBYTE)*src) != '\0') {
			utf8_size = 1 + utf8_trailing_bytes[byte];
			if ((dst + utf8_size) >= dst_end) break;
			switch (utf8_size) {
			case 4: *dst++ = *src++;
			case 3: *dst++ = *src++;
			case 2: *dst++ = *src++;
			case 1: *dst++ = *src++;
			}
		}
		*dst = '\0';
	}
	while ((byte = (UBYTE)*src) != '\0') {
		utf8_size = 1 + utf8_trailing_bytes[byte];
		src += utf8_size;
		dst += utf8_size;
	}
	return dst - dst_start;
}

size_t utf8_strlcat(char *dst, const char *src, size_t dst_size) {
	char *dst_start = dst;
	char *dst_end = dst + dst_size;
	int byte, utf8_size;
	while ((byte = (UBYTE)*dst) != '\0') {
		utf8_size = 1 + utf8_trailing_bytes[byte];
		dst += utf8_size;
	}
	if (dst_end > dst) {
		while ((byte = (UBYTE)*src) != '\0') {
			utf8_size = 1 + utf8_trailing_bytes[byte];
			if ((dst + utf8_size) >= dst_end) break;
			switch (utf8_size) {
			case 4: *dst++ = *src++;
			case 3: *dst++ = *src++;
			case 2: *dst++ = *src++;
			case 1: *dst++ = *src++;
			}
		}
		*dst = '\0';
	}
	while ((byte = (UBYTE)*src) != '\0') {
		utf8_size = 1 + utf8_trailing_bytes[byte];
		src += utf8_size;
		dst += utf8_size;
	}
	return dst - dst_start;
}

