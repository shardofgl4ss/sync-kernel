// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "buddy.h"

struct kslab;

struct kslab_cache {
        const char *identifier;

        bud_order_t order;

        usize obj_size;
        usize alignment;
        usize slab_pages;

        struct kslab *full;
        struct kslab *partial;
        struct kslab *empty;

        u32 empty_slabs;
        u32 slab_objs_per;
        u32 waste;
        u32 reserved;
        u32 metadata;

        u8 max_empty_slabs;
};


extern int slabcache_create(struct kslab_cache *cache, const char *identifier, usize objsize, usize alignment, usize flags);
extern void slabcache_destroy(struct kslab_cache *cache);

extern void *slab_alloc(struct kslab_cache *cache);
extern void slab_free(struct kslab_cache *cache, void *obj);



