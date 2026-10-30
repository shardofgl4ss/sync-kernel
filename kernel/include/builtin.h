// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "macros.h"


/* my attempt to make slightly type generic macros for these builtins. */

_always_inline_
static inline bool checked_add(uint64_t a, uint64_t b, uint64_t *res)
{
        return __builtin_add_overflow(a, b, res);
}

_const_ _always_inline_
static inline u32 _count_trailing_zeroes_32(u32 n)
{
        return __builtin_ctz(n);
}
_const_ _always_inline_
static inline u64 _count_trailing_zeroes_64(u64 n)
{
        return __builtin_ctzll(n);
}
_const_ _always_inline_
static inline u32 _count_leading_zeroes_32(u32 n)
{
        return __builtin_clz(n);
}
_const_ _always_inline_
static inline u64 _count_leading_zeroes_64(u64 n)
{
        return __builtin_clzll(n);
}


#define count_trailing_zeroes(x) \
        (((sizeof((x))) == 8) ? _count_trailing_zeroes_64((u64)(x)) \
        : _count_trailing_zeroes_32((u32)(x)))

#define count_leading_zeroes(x) \
        (((sizeof(x)) == 8) ? _count_leading_zeroes_64((u64)(x)) \
        : _count_leading_zeroes_32((u32)(x)))


_const_ _always_inline_
static inline u32 _ceil_pow2_32(u32 n)
{
        return n == 1 ? 1 : 1U << ((32 - count_leading_zeroes(n - 1)));
}
_const_ _always_inline_
static inline u64 _ceil_pow2_64(u64 n)
{
        return n == 1 ? 1 : 1ULL << ((64 - count_leading_zeroes(n - 1)));
}


_const_ _always_inline_
static inline u32 _floor_pow2_32(u32 n)
{
        return n == 0 ? 0 : 1U << (31 - count_leading_zeroes(n));
}
_const_ _always_inline_
static inline u64 _floor_pow2_64(u64 n)
{
        return n == 0 ? 0 : 1ULL << (63 - count_leading_zeroes(n));
}


// uses floor_pow2_u64 if passed variable is 64 bits, 32 otherwise.
// promotes anything lower then 32 bits to 32 bits.
#define floor_pow2(x) \
        (((sizeof(x)) == 8) ? _floor_pow2_u64((u64)(x)) \
        : _floor_pow2_32((u32)(x)))

// uses ceil_pow2_u64 if passed variable is 64 bits, 32 otherwise.
// promotes anything lower then 32 bits to 32 bits.
#define ceil_pow2(x) \
        (((sizeof(x)) == 8) ? _ceil_pow2_u64((u64)(x)) \
        : _ceil_pow2_32((u32)(x)))



