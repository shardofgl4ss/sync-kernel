//
// Created by SyncShard on 7/25/26.
//

#ifndef KERNEL_PROJECT_KSTRING_H
#define KERNEL_PROJECT_KSTRING_H

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


/* returns ptr to last char, will increment *dest */
__attribute__((always_inline, nonnull(4), used))
static inline unsigned char *rep_movsb(unsigned char *dest,
                                       const unsigned char *src,
                                       size_t n,
                                       size_t *remaining)
{
	__asm__ volatile (
		"rep movsb"
		: "+D" (dest), "+S" (src), "+c" (n)
		:
		: "memory"
	);
	*remaining = n;
	return dest;
}


/* returns ptr to last char, will increment *dest */
__attribute__((always_inline, nonnull(4), used))
static inline unsigned char *repne_movsb(unsigned char *dest,
                                         const unsigned char *src,
                                         size_t n,
                                         size_t *remaining)
{
	__asm__ volatile (
		"repne movsb"
		: "+D" (dest), "+S" (src), "+c" (n)
		:
		: "memory"
	);
	*remaining = n;
	return dest;
}


/* returns ptr to last char, will increment *dest */
__attribute__((always_inline, nonnull(4), used))
static inline unsigned char *rep_stosb(unsigned char *dest,
                                       const unsigned char c,
                                       size_t n,
                                       size_t *remaining)
{
	__asm__ volatile (
		"rep stosb"
		: "+D" (dest), "+c" (n)
		: "a" (c)
		: "memory"
	);
	*remaining = n;
	return dest;
}


__attribute__((always_inline, nonnull(4), used))
static inline unsigned char *repne_stosb(unsigned char *dest,
                                         const unsigned char c,
                                         size_t n,
                                         size_t *remaining)
{
	__asm__ volatile (
		"repne stosb"
		: "+D" (dest), "+c" (n)
		: "a" (c)
		: "memory"
	);
	*remaining = n;
	return dest;
}


__attribute__((always_inline, nonnull(4), used))
static inline unsigned char *repne_scasb(unsigned char *p,
                                         unsigned char v,
                                         size_t n,
                                         size_t *remaining)
{
	__asm__ volatile (
		"repne scasb"
		: "+D" (p), "+c" (n)
		: "a" (v)
		: "memory"
	);
	*remaining = n;
	return p;
}


__attribute__((always_inline, nonnull(4), used))
static inline unsigned char *rep_scasb(unsigned char *p,
                                       unsigned char v,
                                       size_t n,
                                       size_t *remaining)
{
	__asm__ volatile (
		"rep scasb"
		: "+D" (p), "+c" (n)
		: "a"(v)
		: "memory"
	);
	*remaining = n;
	return p;
}


__attribute__((always_inline, used))
static inline void x86_cld(void) { __asm__ volatile ("cld" ::: "cc"); }

__attribute__((always_inline, used))
static inline void x86_std(void) { __asm__ volatile ("std" ::: "cc"); }


#endif //KERNEL_PROJECT_KSTRING_H
