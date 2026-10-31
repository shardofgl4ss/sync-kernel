// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "kmalloc.h"

#include <bootparams.h>
#include <buddy.h>
#include <builtin.h>
#include <macros.h>
#include <pagetbl.h>
#include <panic.h>
#include <slab.h>
#include <types.h>
#include <vma.h>
#include <vmalloc.h>

#define ALIGNMENT       0x10


int kmalloc_init(void)
{
        return 0;
}


void mm_init(void)
{
        if (kern_uhalf_init(&KERN_BOOT_INFO) != 0) panic("kernel upper-half memory failed to initialize!");
        if (vma_subsystem_init() != 0) panic("kernel VMA subsystem failed to initialize!");
        if (vmalloc_init() != 0) panic("kernel vmalloc subsystem failed to initialize!");
        if (kmalloc_init() != 0) panic("kernel kmalloc subsystem failed to initialize!");
}

