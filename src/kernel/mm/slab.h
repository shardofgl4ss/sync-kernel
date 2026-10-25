// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"

extern struct kslab_cache;

extern int slabcache_create(struct kslab_cache *cache, const char *identifier, usize objsize, usize alignment, usize flags);
extern void slabcache_destroy(struct kslab_cache *cache);

extern void *slab_alloc(struct kslab_cache *cache);
extern void slab_free(struct kslab_cache *cache, void *obj);


