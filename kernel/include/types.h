// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <stddef.h>
#include <stdint.h>

/* --------------------------------------------------------------- *
 * rust-like types, bite me. (im moving away from them gradually.) *
 * --------------------------------------------------------------- */

// typedef long long long long long long long long long long long long long long long ssize_t;
typedef long signed ssize_t;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ssize_t isize;


/* 
 * phys_addr_t specifies a hardware RAM address. Not an address in the direct map or anywhere else
 * ex: 0x4658 is a phys_addr_t. 0xFFFF800000004658 is not.
 */
typedef uintptr_t phys_addr_t;

/*
 * virt_addr_t specifies a virtual address, NOT a hardware RAM address. The opposite of phys_addr_t
 * ex: 0xFFFF800000004658 is a virt_addr_t. 0x4658 is not.
 */
typedef uintptr_t virt_addr_t;

/* ---------------------------------------------------- *
 * These are for more assembly-adjacent, or very low -- *
 * level data type stuff. for me it makes more sense to *
 * refer to such bytes/words/qwords/etc, instead of --- *
 * ints/uints, even if they're the same. -------------- *
-* -----------------------------------------Sync Shard- */
typedef u64 qword;
typedef u32 dword;
typedef u16 word;
typedef u8 byte;

