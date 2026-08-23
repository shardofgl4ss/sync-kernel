// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#ifndef _KERNEL_KSTRING_H
#define _KERNEL_KSTRING_H

#include "types.h"

extern size_t strlen(const char *str);

extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern void *mempcpy(void *restrict dest, const void *restrict src, size_t n);
extern void *memccpy(void *restrict dest,
                     const void *restrict src,
                     int c,
                     size_t n);
extern void *memset(void *s, int c, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);

extern char *strcat(char *restrict dest, const char *restrict src);
extern char *strcpy(char *restrict dest, const char *restrict src);
extern char *stpcpy(char *restrict dest, const char *restrict src);

extern char *strncat(char *restrict dst,
                     const char *restrict src,
                     size_t ssize);
extern char *strncpy(char *restrict dest,
                     const char *restrict src,
                     size_t dsize);
extern char *stpncpy(char *restrict dest,
                     const char *restrict src,
                     size_t dsize);

extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strchrnul(const char *s, int c);


#endif //_KERNEL_KSTRING_H
