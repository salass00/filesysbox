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

#ifndef UTF8_H
#define UTF8_H 1

#include <exec/types.h>
#include <stddef.h>

typedef UWORD FbxUCS;

struct FbxAVL {
	struct FbxAVL *left; /* nodes for lower unicodes */
	struct FbxAVL *right; /* nodes for higher unicodes */
	struct FbxAVL *parent;
	FbxUCS         unicode;
	UBYTE          local;
	BYTE           balance; /* > 0 right heavy, < 0 left heavy */
};

LONG utf8_decode_slow(const char **strp);
LONG utf8_decode_fast(const char **strp);
size_t utf8_charcount(const char *str);
char *utf8_charptr(const char *str, size_t n);
int utf8_stricmp(const char *s1, const char *s2);
int utf8_strncmp(const char *s1, const char *s2, size_t n);
int utf8_strnicmp(const char *s1, const char *s2, size_t n);
size_t utf8_strlcpy(char *dst, const char *src, size_t dst_size);
size_t utf8_strlcat(char *dst, const char *src, size_t dst_size);
#ifdef ENABLE_CHARSET_CONVERSION
size_t utf8_to_local(char *dst, const char *src, size_t dst_size, const struct FbxAVL *maptree);
size_t local_to_utf8(char *dst, const char *src, size_t dst_size, const FbxUCS *maptable);
#endif

#endif /* UTF8_H */
