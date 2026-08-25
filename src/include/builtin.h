// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "macros.h"


_always_inline_
static inline bool checked_add(uint64_t a, uint64_t b, uint64_t *res)
{
        return __builtin_add_overflow(a, b, res);
}


_const_ _always_inline_
static inline usize next_pow_two(usize n)
{
        return n == 1 ? 1 : 1 << (64 - __builtin_clzl(n - 1));
}

