// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#ifndef _KERNEL_BUILTIN_H
#define _KERNEL_BUILTIN_H

#include "types.h"
#include "macros.h"

_SY_PRIMITIVE bool checked_add(uint64_t a, uint64_t b, uint64_t *res)
{
        return __builtin_add_overflow(a, b, res);
}

#endif

