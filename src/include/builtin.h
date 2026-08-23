// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "macros.h"

_SY_PRIMITIVE bool checked_add(uint64_t a, uint64_t b, uint64_t *res)
{
        return __builtin_add_overflow(a, b, res);
}

