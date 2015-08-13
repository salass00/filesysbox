/*
 * Copyright (c) 2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include <string.h>

#ifndef __AROS__
size_t strlcpy(char *dst, const char *src, size_t dst_size) {
	char *dst_start = dst;
	char *dst_end = dst_start + dst_size;

	if (dst_end > dst) {
		while ((*dst = *src) != '\0') {
			if (++dst == dst_end) {
				*--dst = '\0';
				break;
			}
			src++;
		}
	}
	dst += strlen(src);
	return dst - dst_start;
}

size_t strlcat(char *dst, const char *src, size_t dst_size) {
	char *dst_start = dst;
	char *dst_end = dst_start + dst_size;

	dst += strlen(dst);
	if (dst_end > dst) {
		while ((*dst = *src) != '\0') {
			if (++dst == dst_end) {
				*--dst = '\0';
				break;
			}
			src++;
		}
	}
	dst += strlen(src);
	return dst - dst_start;
}
#endif

