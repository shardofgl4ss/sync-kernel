// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "macros.h"
#include "types.h"
#include "pframe.h"
#include "multiboot2.h"

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

        usize region_count;

        // includes unusable ram.
        usize total_ram;
        usize usable_ram;

};

extern struct preinit_allocator preinit_pfa;
extern struct multiboot_tag_mmap *multiboot2_tmmap;

