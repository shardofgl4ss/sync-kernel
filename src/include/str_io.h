// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "macros.h"

/* returns ptr to last char, will increment *dest */
_SY_PRIMITIVE unsigned char *rep_movsb(unsigned char *dest,
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
	if (remaining) *remaining = n;
	return dest;
}


/* returns ptr to last char, will increment *dest */
_SY_PRIMITIVE unsigned char *repne_movsb(unsigned char *dest,
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
	if (remaining) *remaining = n;
	return dest;
}


/* returns ptr to last char, will increment *dest */
_SY_PRIMITIVE unsigned char *rep_stosb(unsigned char *dest,
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
	if (remaining) *remaining = n;
	return dest;
}


_SY_PRIMITIVE unsigned char *repne_stosb(unsigned char *dest,
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
	if (remaining) *remaining = n;
	return dest;
}


_SY_PRIMITIVE unsigned char *repne_scasb(unsigned char *p,
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
	if (remaining) *remaining = n;
	return p;
}


_SY_PRIMITIVE unsigned char *rep_scasb(unsigned char *p,
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
	if (remaining) *remaining = n;
	return p;
}


_SY_PRIMITIVE void x86_cld(void) { __asm__ volatile ("cld" ::: "cc"); }
_SY_PRIMITIVE void x86_std(void) { __asm__ volatile ("std" ::: "cc"); }

