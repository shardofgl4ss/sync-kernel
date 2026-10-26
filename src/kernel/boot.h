// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "macros.h"
#include "types.h"
#include "multiboot2.h"

#include "mm/buddy.h"

typedef struct preinit_region {
        page_frame_t *top;

        // usually points to the physmem_pregion_t.
        void *base;

        usize cur_frames;
        usize max_frames;
} physmem_pregion_t;

struct preinit_allocator {
        // KPAGE_ERR/(void *)-1 is the terminator.
        physmem_pregion_t *region[PFRAME_MAX_REGIONS];

        // boot_reserved denotes where the initial pagetables begin and end.
        // void *boot_reserved_start;
        // void *boot_reserved_end;
        // usize boot_pagetable_count;

        usize region_count;

        // includes unusable ram.
        usize total_ram;
        usize usable_ram;

};

extern struct preinit_allocator preinit_pfa;
extern struct multiboot_tag_mmap *multiboot2_tmmap;

