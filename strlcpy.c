/*
 * Copyright (c) 2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include <string.h>

#ifndef __AROS__
size_t strlcpy(char *dst, const char *src, size_t size) {
	char *d = dst;
	const char *s = src;
	size_t n = size;

	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == '\0')
				break;
		} while (--n != 0);
	}

	if (n == 0) {
		if (size != 0)
			*d = '\0';

		while (*s++ != '\0');
	}

	return s - src - 1;
}

size_t strlcat(char *dst, const char *src, size_t size) {
	char *d = dst;
	const char *s = src;
	size_t n = size;
	size_t dlen;

	while (n-- != 0 && *d != '\0')
		d++;

	dlen = d - dst;
	n = size - dlen;

	if (n == 0)
		return dlen + strlen(s);

	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}

	*d = '\0';

	return dlen + (s - src);
}
#endif

