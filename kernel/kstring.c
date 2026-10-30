// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "kstring.h"
#include "io.h"
#include "str_io.h"


size_t strlen(const char *restrict str)
{
	size_t len = 0;

	while (*str++) {
		len++;
	}

	return len;
}


size_t strnlen(const char *restrict str, const size_t maxlen)
{
	size_t i = 0;
	for (; (i < maxlen) && str[i]; i++);

	return i;
}


void *memcpy(void *restrict dest, const void *restrict src, const size_t n)
{
	unsigned char *destination = dest;
	unsigned const char *source = src;
	size_t remaining = 0;

	x86_cld();
	rep_movsb(destination, source, n, &remaining);

	return dest;
}


void *mempcpy(void *restrict dest, const void *restrict src, const size_t n)
{
	size_t remaining = 0;

	// forward
	rep_movsb(dest, src, n, &remaining);
	return dest;
}


void *memccpy(void *restrict dest,
              const void *restrict src,
              const int c,
              const size_t n)
{
	unsigned char *d = dest;
	unsigned const char *s = src;

	for (size_t i = 0; i < n; i++) {
		*d = *s;
		if (*s == (unsigned char)c) {
			return ++d;
		}
		d++;
		s++;
	}
	return NULL;
}


void *memset(void *s, const int c, const size_t n)
{
	unsigned char *str = s;
	size_t remaining = 0;

	x86_cld();
	rep_stosb(str, c, n, &remaining);

	return s;
}


void *memmove(void *dest, const void *src, const size_t n)
{
	unsigned char *destination = dest;
	const unsigned char *source = src;

	if (destination == source || n == 0)
		return dest;

	if (destination < source) {
		for (size_t i = 0; i < n; i++)
			destination[i] = source[i];
	} else if (destination > source) {
		for (size_t i = n; i != 0; i--)
			destination[i - 1] = source[i - 1];
	}

	return dest;
}


char *strcat(char *restrict dest, const char *restrict src)
{
	stpcpy(dest + strlen(dest), src);
	return dest;
}


char *strcpy(char *restrict dest, const char *restrict src)
{
	stpcpy(dest, src);
	return dest;
}


char *stpcpy(char *restrict dest, const char *restrict src)
{
	char *s = mempcpy(dest, src, strlen(src));
	*s = '\0';

	return s;
}


char *strncat(char *restrict dst, const char *restrict src, const size_t ssize)
{
	#define strnul(s)	((s) + strlen(s))
	stpcpy(memcpy(strnul(dst), src, strnlen(src, ssize)), "");
	return dst;
}


char *strncpy(char *restrict dest, const char *restrict src, const size_t dsize)
{
	stpncpy(dest, src, dsize);
	return dest;
}


extern char *stpncpy(char *restrict dest,
                     const char *restrict src,
                     const size_t dsize)
{
	const size_t len = strnlen(src, dsize);
	return memset(mempcpy(dest, src, len), 0, dsize - len);
}


char *strchr(const char *s, const int c)
{
	size_t remaining = 0;
	unsigned char *p = (unsigned char *)s;

	x86_cld();
	unsigned char *end = repne_scasb(p,
	                                 (unsigned char)c,
	                                 strlen(s) + 1,
	                                 &remaining);
	if (remaining != 0)
		return (char *)(end - 1);
	return NULL;
}


char *strrchr(const char *s, const int c)
{
	size_t remaining = 0;
	const size_t strsz = strlen(s);
	unsigned char *p = (unsigned char *)s;

	x86_std();
	unsigned char *end = repne_scasb(p + strsz,
	                                 (unsigned char)c,
	                                 strsz + 1,
	                                 &remaining);
	x86_cld(); // I *think* this needs to be cleared.

	if (remaining != 0)
		return (char *)(end + 1);
	return NULL;
}


char *strchrnul(const char *s, const int c)
{
	size_t remaining = 0;
	unsigned char *p = (unsigned char *)s;

	x86_cld();
	unsigned char *end = repne_scasb(p,
	                                 (unsigned char)c,
	                                 strlen(s) + 1,
	                                 &remaining);
	return (char *)(end - 1);
}
