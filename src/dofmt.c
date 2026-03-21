/*
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef NODEBUG
#include "filesysbox_internal.h"
#include <exec/types.h>
#include <string.h>
#include <stdarg.h>

static void reverse(char *str, size_t len) {
	char *start = str;
	char *end = str + len - 1;
	char tmp;

	while (start < end) {
		tmp = *end;
		*end-- = *start;
		*start++ = tmp;
	}
}

static size_t itoa(unsigned num, char *dst, unsigned base,
	BOOL issigned, BOOL addplus, BOOL uppercase)
{
	char a = uppercase ? 'A' : 'a';
	char negative = FALSE;
	char *d = dst;
	size_t len;

	if (num == 0) {
		*d++ = '0';
		return d - dst;
	}

	if (issigned && (int)num < 0 && base == 10) {
		negative = TRUE;
		num = -num;
	}

	while (num != 0) {
		unsigned rem = num % base;
		num /= base;
		*d++ = (rem > 9) ? (rem - 10 + a) : (rem + '0');
	}

	if (negative)
		*d++ = '-';
	else if (addplus)
		*d++ = '+';

	len = d - dst;
	reverse(dst, len);
	return len;
}

static size_t ltoa(unsigned long num, char *dst, unsigned base,
	BOOL issigned, BOOL addplus, BOOL uppercase)
{
	char a = uppercase ? 'A' : 'a';
	char negative = FALSE;
	char *d = dst;
	size_t len;

	if (num == 0) {
		*d++ = '0';
		return d - dst;
	}

	if (issigned && (long)num < 0 && base == 10) {
		negative = TRUE;
		num = -num;
	}

	while (num != 0) {
		unsigned rem = num % base;
		num /= base;
		*d++ = (rem > 9) ? (rem - 10 + a) : (rem + '0');
	}

	if (negative)
		*d++ = '-';
	else if (addplus)
		*d++ = '+';

	len = d - dst;
	reverse(dst, len);
	return len;
}

static size_t lltoa(unsigned long long num, char *dst, unsigned base,
	BOOL issigned, BOOL addplus, BOOL uppercase)
{
	char a = uppercase ? 'A' : 'a';
	char negative = FALSE;
	char *d = dst;
	size_t len;

	if (num == 0) {
		*d++ = '0';
		return d - dst;
	}

	if (issigned && (signed long long)num < 0 && base == 10) {
		negative = TRUE;
		num = -num;
	}

	while (num != 0) {
		unsigned rem = num % base;
		num /= base;
		*d++ = (rem > 9) ? (rem - 10 + a) : (rem + '0');
	}

	if (negative)
		*d++ = '-';
	else if (addplus)
		*d++ = '+';

	len = d - dst;
	reverse(dst, len);
	return len;
}

#define PUTC(ch) \
	do { \
		if (cb((ch), cb_data) == 0) \
			count++; \
		else \
			return -1; \
	} while (0)

#define FBXF_DOFMT_LEFTJUSTIFY 0x01
#define FBXF_DOFMT_ADDPLUS     0x02
#define FBXF_DOFMT_ALTERNATE   0x04
#define FBXF_DOFMT_LONG        0x08
#define FBXF_DOFMT_LONGLONG    0x10
#define FBXF_DOFMT_SHORT       0x20
#define FBXF_DOFMT_CHAR        0x40

size_t FbxDoFmt(FbxPutCFunc cb, void *cb_data, const char *fmt, va_list arg) {
	size_t count = 0;
	char ch;

	while ((ch = *fmt++) != '\0') {
		if (ch != '%') {
			PUTC(ch);
		} else {
			unsigned int flags = 0;
			char lead = ' ';
			size_t width = 0;
			size_t limit = 0;
			char tmp[128];
			const char *src;
			size_t len;

			if ((ch = *fmt++) == '\0')
				return count;

			while (TRUE) {
				if (ch == '-')
					flags |= FBXF_DOFMT_LEFTJUSTIFY;
				else if (ch == '+')
					flags |= FBXF_DOFMT_ADDPLUS;
				else if (ch == '#')
					flags |= FBXF_DOFMT_ALTERNATE;
				else if (ch == '0')
					lead = '0';
				else
					break;
				if ((ch = *fmt++) == '\0')
					return count;
			}

			while (ch >= '0' && ch <= '9') {
				width = 10 * width + (ch - '0');
				if ((ch = *fmt++) == '\0')
					return count;
			}

			if (ch == '.') {
				if ((ch = *fmt++) == '\0')
					return count;

				while (ch >= '0' && ch <= '9') {
					limit = 10 * limit + (ch - '0');
					if ((ch = *fmt++) == '\0')
						return count;
				}
			}

			if (ch == 'z') {
				if ((ch = *fmt++) == '\0')
					return count;
				if (sizeof(size_t) == sizeof(long long) && sizeof(long long) > sizeof(long))
					flags |= FBXF_DOFMT_LONGLONG;
				else if (sizeof(size_t) == sizeof(long) && sizeof(long) > sizeof(int))
					flags |= FBXF_DOFMT_LONG;
			} else if (ch == 'h') {
				if ((ch = *fmt++) == '\0')
					return count;
				if (ch == 'h') {
					if ((ch = *fmt++) == '\0')
						return count;
					flags |= FBXF_DOFMT_CHAR;
				} else {
					flags |= FBXF_DOFMT_SHORT;
				}
			} else if (ch == 'l') {
				if ((ch = *fmt++) == '\0')
					return count;
				if (ch == 'l') {
					if ((ch = *fmt++) == '\0')
						return count;
					if (sizeof(long long) > sizeof(long))
						flags |= FBXF_DOFMT_LONGLONG;
					else
						flags |= FBXF_DOFMT_LONG;
				} else if (sizeof(long) > sizeof(int)) {
					flags |= FBXF_DOFMT_LONG;
				}
			}

			switch (ch) {
				case '%':
					PUTC('%');
					break;
				case 'D':
				case 'd':
				case 'I':
				case 'i':
				{
					BOOL addplus = (flags & FBXF_DOFMT_ADDPLUS) ? TRUE : FALSE;
					BOOL uppercase = (ch == 'D' || ch == 'I') ? TRUE : FALSE;
					if (flags & FBXF_DOFMT_LONGLONG)
						len = lltoa(va_arg(arg, long long), tmp, 10, TRUE, addplus, uppercase);
					else if (flags & FBXF_DOFMT_LONG)
						len = ltoa(va_arg(arg, long), tmp, 10, TRUE, addplus, uppercase);
					else
						len = itoa(va_arg(arg, int), tmp, 10, TRUE, addplus, uppercase);

					src = tmp;
					if (width > len)
						width -= len;
					else
						width = 0;

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) == 0)
						while (width--)
							PUTC(lead);

					while (len--)
						PUTC(*src++);

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) != 0)
						while (width--)
							PUTC(' ');
					break;
				}
				case 'U':
				case 'u':
				{
					BOOL uppercase = (ch == 'X') ? TRUE : FALSE;
					if (flags & FBXF_DOFMT_LONGLONG)
						len = lltoa(va_arg(arg, long long), tmp, 10, FALSE, FALSE, uppercase);
					else if (flags & FBXF_DOFMT_LONG)
						len = ltoa(va_arg(arg, long), tmp, 10, FALSE, FALSE, uppercase);
					else
						len = itoa(va_arg(arg, int), tmp, 10, FALSE, FALSE, uppercase);

					src = tmp;
					if (width > len)
						width -= len;
					else
						width = 0;

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) == 0)
						while (width--)
							PUTC(lead);

					while (len--)
						PUTC(*src++);

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) != 0)
						while (width--)
							PUTC(' ');
					break;
				}
				case 'O':
				case 'o':
				{
					BOOL uppercase = (ch == 'O') ? TRUE : FALSE;
					if (flags & FBXF_DOFMT_LONGLONG)
						len = lltoa(va_arg(arg, long long), tmp, 8, FALSE, FALSE, uppercase);
					else if (flags & FBXF_DOFMT_LONG)
						len = ltoa(va_arg(arg, long), tmp, 8, FALSE, FALSE, uppercase);
					else
						len = itoa(va_arg(arg, int), tmp, 8, FALSE, FALSE, uppercase);

					src = tmp;
					if (width > len)
						width -= len;
					else
						width = 0;

					if ((flags & FBXF_DOFMT_ALTERNATE) && tmp[0] != '0') {
						PUTC('0');
					}

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) == 0)
						while (width--)
							PUTC(lead);

					while (len--)
						PUTC(*src++);

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) != 0)
						while (width--)
							PUTC(' ');
					break;
				}
				case 'X':
				case 'x':
				{
					BOOL uppercase = (ch == 'X') ? TRUE : FALSE;
					if (flags & FBXF_DOFMT_LONGLONG)
						len = lltoa(va_arg(arg, long long), tmp, 16, FALSE, FALSE, uppercase);
					else if (flags & FBXF_DOFMT_LONG)
						len = ltoa(va_arg(arg, long), tmp, 16, FALSE, FALSE, uppercase);
					else
						len = itoa(va_arg(arg, int), tmp, 16, FALSE, FALSE, uppercase);

					src = tmp;
					if (width > len)
						width -= len;
					else
						width = 0;

					if ((flags & FBXF_DOFMT_ALTERNATE) && tmp[0] != '0') {
						PUTC('0');
						PUTC('x');
					}

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) == 0)
						while (width--)
							PUTC(lead);

					while (len--)
						PUTC(*src++);

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) != 0)
						while (width--)
							PUTC(' ');
					break;
				}
				case 'P':
				case 'p':
				{
					BOOL uppercase = (ch == 'P') ? TRUE : FALSE;
					if (sizeof(void *) == sizeof(long long) && sizeof(long long) > sizeof(long))
						len = lltoa(va_arg(arg, long long), tmp, 16, FALSE, FALSE, uppercase);
					else if (sizeof(void *) == sizeof(long) && sizeof(long) > sizeof(int))
						len = ltoa(va_arg(arg, long), tmp, 16, FALSE, FALSE, uppercase);
					else
						len = itoa(va_arg(arg, int), tmp, 16, FALSE, FALSE, uppercase);

					src = tmp;
					width = 2*sizeof(void *);
					lead = '0';
					if (width > len)
						width -= len;
					else
						width = 0;

					PUTC('0');
					PUTC('x');

					while (width--)
						PUTC(lead);

					while (len--)
						PUTC(*src++);
					break;
				}
				case 'S':
				case 's':
					src = va_arg(arg, const char *);
					if (src == NULL)
						src = "(null)";

					len = strlen(src);

					if (limit != 0)
						len = min(len, limit);

					if (width > len)
						width -= len;
					else
						width = 0;

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) == 0)
						while (width--)
							PUTC(' ');

					while (len--)
						PUTC(*src++);

					if ((flags & FBXF_DOFMT_LEFTJUSTIFY) != 0)
						while (width--)
							PUTC(' ');
					break;
			}
		}
	}

	return count;
}
#endif /* !NODEBUG */

