/*
 * UTF-8 support routines
 *
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 *
 * The utf8_decode_slow() and utf8_decode_fast() implementations are based on:
 * Basic UTF-8 manipulation routines by Jeff Bezanson.
 * Placed in the public domain Autumn 2005.
 */

#include "filesysbox_internal.h"
#include <stdint.h> /* For UINT16_MAX */

static const unsigned char utf8_trailing_bytes[256] = {
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

static const unsigned char utf8_lb_masks[6] = {
	0x7F, 0x1F, 0x0F, 0x07, 0x03, 0x01
};

static const ULONG utf8_first_codes[6] = {
	0x0, 0x80, 0x800, 0x10000, 0x200000, 0x4000000
};

// slow, validating decode function
LONG utf8_decode_slow(const char **strp) {
	const unsigned char *str = (const unsigned char *)*strp;
	unsigned char byte, ntb;
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
	const unsigned char *str = (const unsigned char *)*strp;
	unsigned char ntb = utf8_trailing_bytes[*str];
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
	while ((byte = (unsigned char)*str) != '\0') {
		str += 1 + utf8_trailing_bytes[byte];
		len++;
	}
	return len;
}

char *utf8_strskip(const char *str, size_t n) {
	int byte;
	if (n == 0) return (char *)str;
	while ((byte = (unsigned char)*str) != '\0') {
		str += 1 + utf8_trailing_bytes[byte];
		if (--n == 0) return (char *)str;
	}
	return NULL;
}

int utf8_stricmp(const char *s1, const char *s2) {
	LONG c1, c2;
	do {
		c1 = ucs4_toupper(utf8_decode_fast(&s1));
		c2 = ucs4_toupper(utf8_decode_fast(&s2));
	} while (c1 != '\0' && c1 == c2);
	return c1 - c2;
}

int utf8_strncmp(const char *s1, const char *s2, size_t n) {
	LONG c1, c2;
	if (n == 0) return 0;
	do {
		c1 = utf8_decode_fast(&s1);
		c2 = utf8_decode_fast(&s2);
	} while (c1 != '\0' && --n > 0 && c1 == c2);
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
		while ((byte = (unsigned char)*src) != '\0') {
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
	while ((byte = (unsigned char)*src) != '\0') {
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
	while ((byte = (unsigned char)*dst) != '\0') {
		utf8_size = 1 + utf8_trailing_bytes[byte];
		dst += utf8_size;
	}
	if (dst_end > dst) {
		while ((byte = (unsigned char)*src) != '\0') {
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
	while ((byte = (unsigned char)*src) != '\0') {
		utf8_size = 1 + utf8_trailing_bytes[byte];
		src += utf8_size;
		dst += utf8_size;
	}
	return dst - dst_start;
}

#ifdef ENABLE_CHARSET_CONVERSION
#include <string.h>

static int escape_unicode(char *seq, ULONG unicode, const char *b32tab)
{
	if (unicode < 0x80)
	{
		if (seq != NULL)
		{
			seq[0] = '%';
			seq[1] = b32tab[(1<<2)|(unicode>>5)];
			seq[2] = b32tab[(unicode)&0x1f];
		}
		return 3;
	}
	else if (unicode < 0x800)
	{
		if (seq != NULL)
		{
			seq[0] = '%';
			seq[1] = b32tab[(2<<2)|(unicode>>10)];
			seq[2] = b32tab[(unicode>>5)&0x1f];
			seq[3] = b32tab[(unicode)&0x1f];
		}
		return 4;
	}
	else if (unicode < 0x10000)
	{
		if (seq != NULL)
		{
			seq[0] = '%';
			seq[1] = b32tab[(3<<2)|(unicode>>14)];
			seq[2] = b32tab[(unicode>>9)&0x1f];
			seq[3] = b32tab[(unicode>>4)&0x1f];
			seq[4] = b32tab[(unicode<<1)&0x1f];
		}
		return 5;
	}
	else
	{
		if (seq != NULL)
		{
			seq[0] = '%';
			seq[1] = b32tab[(4<<2)|(unicode>>19)];
			seq[2] = b32tab[(unicode>>14)&0x1f];
			seq[3] = b32tab[(unicode>>9)&0x1f];
			seq[4] = b32tab[(unicode>>4)&0x1f];
			seq[5] = b32tab[(unicode<<1)&0x1f];
		}
		return 6;
	}
}

size_t utf8_to_local(char *dst, const char *src, size_t dst_size, const struct FbxAVL *maptree)
{
	static const char b32tab[32] =
	{
		'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
		'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V'
	};
	char *dst_start = dst;
	char *dst_end = dst + dst_size;
	ULONG unicode = ~0;

	if (dst < dst_end)
	{
		dst_end--;

		while (dst < dst_end && (unicode = utf8_decode_fast(&src)) != '\0')
		{
			if (unicode < 0x80)
			{
				*dst++ = unicode;
			}
			else
			{
				int local = '\0';

				if (maptree == NULL)
				{
					/* Assume latin-1 */
					if (unicode < 0x100)
						local = unicode;
				}
				else if (sizeof(FbxUCS) == 4 || unicode <= UINT16_MAX)
				{
					const struct FbxAVL *n = maptree;
					while (n != NULL)
					{
						if (unicode == n->unicode)
						{
							local = n->local;
							break;
						}

						if (unicode < n->unicode)
							n = n->left;
						else
							n = n->right;
					}
				}

				if (local != '\0')
				{
					*dst++ = local;
				}
				else
				{
					char seq[6];
					int seqlen, copylen;

					seqlen = escape_unicode(seq, unicode, b32tab);

					copylen = seqlen;
					if (copylen > (dst_end - dst))
						copylen = dst_end - dst;

					memcpy(dst, seq, copylen);
					dst += seqlen;
				}
			}
		}

		if (dst < dst_end)
			*dst = '\0';
		else
			*dst_end = '\0';

		if (unicode == '\0')
			return dst - dst_start;
	}

	while ((unicode = utf8_decode_fast(&src)) != '\0')
	{
		if (unicode < 0x80)
		{
			dst++;
		}
		else
		{
			int local = '\0';

			if (maptree == NULL)
			{
				/* Assume latin-1 */
				if (unicode < 0x100)
					local = unicode;
			}
			else if (sizeof(FbxUCS) == 4 || unicode <= UINT16_MAX)
			{
				const struct FbxAVL *n = maptree;
				while (n != NULL)
				{
					if (unicode == n->unicode)
					{
						local = n->local;
						break;
					}

					if (unicode < n->unicode)
						n = n->left;
					else
						n = n->right;
				}
			}

			if (local != '\0')
			{
				dst++;
			}
			else
			{
				dst += escape_unicode(NULL, unicode, NULL);
			}
		}
	}

	return dst - dst_start;
}

static LONG unescape_unicode(const char **strp, const char *b32tab)
{
	const char *s = *strp;
	unsigned c, b;
	unsigned len;
	ULONG unicode;

	if (b32tab[c = (unsigned char)*s++] == 0)
		return -1; /* Not valid base32 */
	b = c - b32tab[c];
	len = b >> 2;
	if (len >= 1 && len <= 4)
	{
		switch (len)
		{
			case 1:
				unicode = (b&0x3)<<5;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b;
				break;

			case 2:
				unicode = (b&0x3)<<10;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b<<5;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b;
				break;

			case 3:
				unicode = (b&0x3)<<14;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b<<9;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b<<4;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				if (b&1)
					return -1; /* Not a valid escape code */
				unicode |= b>>1;
				break;

			case 4:
				unicode = (b&0x3)<<19;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b<<14;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b<<9;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				unicode |= b<<4;
				if (b32tab[c = (unsigned char)*s++] == 0)
					return -1; /* Not valid base32 */
				b = c - b32tab[c];
				if (b&1)
					return -1; /* Not a valid escape code */
				unicode |= b>>1;
				break;
		}
	}
	else
		return -1; /* Not a valid length */

	if (unicode < utf8_first_codes[len-1] ||
		(unicode >= 0xD800 && unicode <= 0xDFFF) ||
		unicode == 0xFFFE || unicode == 0xFFFF ||
		unicode > 0x10FFFF)
	{
		return -1; /* Not a valid unicode */
	}

	*strp = s;
	return unicode;
}

static int encode_unicode(char *seq, ULONG unicode)
{
	if (unicode < 0x80)
	{
		if (seq != NULL)
		{
			seq[0] = unicode;
		}
		return 1;
	}
	else if (unicode < 0x800)
	{
		if (seq != NULL)
		{
			seq[0] = 0xC0 | ((unicode >> 6) & 0x1F);
			seq[1] = 0x80 | (unicode & 0x3F);
		}
		return 2;
	}
	else if (unicode < 0x10000)
	{
		if (seq != NULL)
		{
			seq[0] = 0xE0 | ((unicode >> 12) & 0x0F);
			seq[1] = 0x80 | ((unicode >> 6) & 0x3F);
			seq[2] = 0x80 | (unicode & 0x3F);
		}
		return 3;
	}
	else
	{
		if (seq != NULL)
		{
			seq[0] = 0xF0 | ((unicode >> 18) & 0x07);
			seq[1] = 0x80 | ((unicode >> 12) & 0x3F);
			seq[2] = 0x80 | ((unicode >> 6) & 0x3F);
			seq[3] = 0x80 | (unicode & 0x3F);
		}
		return 4;
	}
}

size_t local_to_utf8(char *dst, const char *src, size_t dst_size, const FbxUCS *maptable)
{
	static const char b32tab[256] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,48,48,48,48,48,48,48,48,48,48, 0, 0, 0, 0, 0, 0,
		0,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55,55, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};
	char *dst_start = dst;
	char *dst_end = dst + dst_size;
	int local = ~0;

	if (dst < dst_end)
	{
		dst_end--;

		while (dst < dst_end && (local = (unsigned char)*src++) != '\0')
		{
			if (local != '%')
			{
				ULONG unicode;

				if (maptable == NULL)
					unicode = local; /* Assume latin-1 */
				else
					unicode = maptable[local];

				if (unicode < 0x80)
				{
					*dst++ = unicode;
				}
				else
				{
					char seq[4];
					int seqlen;

					seqlen = encode_unicode(seq, unicode);

					if (seqlen > (dst_end - dst))
					{
						dst += seqlen;
						break;
					}

					memcpy(dst, seq, seqlen);
					dst += seqlen;
				}
			}
			else
			{
				const char *s = src;
				ULONG unicode;

				unicode = unescape_unicode(&s, b32tab);
				if ((LONG)unicode >= 0x80)
				{
					char seq[4];
					int seqlen;

					seqlen = encode_unicode(seq, unicode);

					if (seqlen > (dst_end - dst))
					{
						dst += seqlen;
						break;
					}

					memcpy(dst, seq, seqlen);
					dst += seqlen;
					src = s;
				}
				else
				{
					*dst++ = '%';
				}
			}
		}

		if (dst < dst_end)
			*dst = '\0';
		else
			*dst_end = '\0';

		if (local == '\0')
			return dst - dst_start;
	}

	while ((local = (unsigned char)*src++) != '\0')
	{
		if (local != '%')
		{
			ULONG unicode;

			if (maptable == NULL)
				unicode = local; /* Assume latin-1 */
			else
				unicode = maptable[local];

			if (unicode < 0x80)
			{
				dst++;
			}
			else
			{
				dst += encode_unicode(NULL, unicode);
			}
		}
		else
		{
			const char *s = src;
			ULONG unicode;

			unicode = unescape_unicode(&s, b32tab);
			if ((LONG)unicode >= 0x80)
			{
				dst += encode_unicode(NULL, unicode);
				src = s;
			}
			else
			{
				dst++;
			}
		}
	}

	return dst - dst_start;
}
#endif

