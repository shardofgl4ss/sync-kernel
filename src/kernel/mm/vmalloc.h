// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"

extern int vmalloc_init(void);
extern void *kmap(phys_addr_t phy, usize len, u64 flags);
extern void *ioremap(phys_addr_t phy, usize len);
