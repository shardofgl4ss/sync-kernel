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
#include "vma.h"
#include "vmalloc.h"

#define ALIGNMENT       0x10


int kmalloc_init(void)
{
}


void mm_init(void)
{
        if (!kern_uhalf_init()) panic("kernel upper-half memory failed to initialize!");
        if (!vma_subsystem_init()) panic("kernel VMA subsystem failed to initialize!");
        if (!vmalloc_init()) panic("kernel vmalloc subsystem failed to initialize!");
        if (!kmalloc_init()) panic("kernel kmalloc subsystem failed to initialize!");
}

