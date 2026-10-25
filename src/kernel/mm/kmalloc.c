// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "kmalloc.h"

#include "builtin.h"
#include "macros.h"
#include "pagetbl.h"
#include "panic.h"
#include "types.h"

#include "buddy.h"
#include "slab.h"
#include "vmalloc.h"

#define ALIGNMENT       0x10


void kmalloc_init(void)
{
}

