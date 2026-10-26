// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "slab.h"

#include "buddy.h"
#include "kstring.h"
#include "macros.h"
#include "panic.h"
#include "types.h"

#define KSLAB_PRESET_MAX_EMPTY  4


struct kslab {
        struct kslab_cache *cache;

        struct kslab *next;
        struct kslab *prev;

        void *pool;
        void *freelist;

        u32 free_count;
        // u32 obj_total;
};


static struct kslab *new_slab_init(struct kslab_cache *cache)
{
        if (unlikely(!cache)) return KPAGE_ERR;

        char *rmem = kbud_alloc(cache->order);

        if (rmem == KPAGE_ERR) {
                return KPAGE_ERR;
        }

        memset(rmem, 0, buddy_order_to_val(cache->order));
        struct kslab *s = (void *)rmem;

        struct kslab **pe = &cache->empty;

        while (*pe) {
                pe = &((*pe)->next);
        }
        
        *pe = s;

        // s->obj_total = cache->
        s->cache = cache;
        s->pool = (void *)((char *)s + cache->reserved);
        s->freelist = s->pool;
        s->free_count = cache->slab_objs_per;

        char *curr = (char *)s->pool;
        char *next;

        for (usize i = 0; i < cache->slab_objs_per - 1; i++) {
                next = curr + cache->obj_size;
                *(void **)curr = (void *)next;
                curr = next;
        }

        *(void **)curr = NULL;
        
        return s;
}




int slabcache_create(struct kslab_cache *cache, const char *identifier, usize objsize, usize alignment, usize flags)
{
        static constexpr int max_order = BUDDY_ORDER_32K;
        static constexpr usize slab_hdr_sz = sizeof(struct kslab);

        if (unlikely(!cache)) return -1;

        bud_order_t ctx_order = {};
        u32 ctx_reserved = {};
        usize ctx_count = {};
        usize ctx_waste = (usize)-1; 


        enum buddy_block_order order = 0;

        for (int i = 0; i < max_order; i++) {
                const usize size = (1ULL << i) * PAGESIZE;
                const usize pad = (alignment - (slab_hdr_sz % alignment)) % alignment;
                const usize reserved = slab_hdr_sz + pad;

                if (unlikely(size <= reserved)) continue;
                const usize rem = size - reserved;
                const usize objs = rem / objsize;

                if (objs < 8 && i < max_order) continue;
                if (objs == 0) continue;
                const usize waste = size - (reserved + (objs * objsize));

                if (waste < ctx_waste) {
                        ctx_order = i;
                        ctx_count = objs;
                        ctx_waste = waste;
                        ctx_reserved = reserved;
                }
        }

        if (ctx_waste == (usize)-1) return 1;

        const usize order_bytes = buddy_order_to_val(ctx_order);

        cache->identifier = identifier;
        cache->slab_pages = order_bytes / PAGESIZE;
        cache->alignment = alignment;
        cache->obj_size = objsize;
        cache->metadata = flags;

        cache->order = ctx_order;
        cache->waste = ctx_waste;
        cache->reserved = ctx_reserved;
        cache->slab_objs_per = ctx_count;

        cache->empty_slabs++;
        cache->max_empty_slabs = KSLAB_PRESET_MAX_EMPTY;

        if (new_slab_init(cache) == KPAGE_ERR) {
                return -1;
        }

        return 0;
}




void slabcache_destroy(struct kslab_cache *cache)
{
        struct kslab **ps[] = {
                &cache->full,
                &cache->partial,
                &cache->empty,
        };

        for (int i = 0; i < 3; i++) {
                while (*(ps[i])) {
                        struct kslab *s = *(ps[i]);
                        ps[i] = &((*(ps[i]))->next);
                        kbud_free((phys_addr_t)s, cache->order);
                        // slabs are always at the very base of the allocation.
                }
        }

        cache->obj_size = (usize)-1;
        cache->slab_pages = 0;
        cache->order = -1;
}




static void slab_remove_node(struct kslab_cache *cache, struct kslab **from, struct kslab *target)
{
        struct kslab **p = from;

        while (*p && *p != target) {
                p = &((*p)->next);
        }

        if (*p) *p = target->next;

        kbud_free((phys_addr_t)target, cache->order);
}




static void slab_relink_node(struct kslab **from, struct kslab **to, struct kslab *target)
{
        struct kslab **p = from;

        while (*p && *p != target) {
                p = &((*p)->next);
        }

        if (*p) *p = target->next;
        target->next = NULL;
        p = to;

        while (*p) {
                p = &((*p)->next);
        }

        *p = target;
}




static void *slab_partial_alloc(struct kslab_cache *cache)
{
        if (!cache->partial) return KPAGE_ERR;

        struct kslab *slab = cache->partial;

        if (!(--slab->free_count)) {
                slab_relink_node(&cache->partial, &cache->full, slab);
        }

        void **retalloc = (void **)slab->freelist;
        slab->freelist = *retalloc;

        return (void *)retalloc;

}
static void *slab_empty_alloc(struct kslab_cache *cache)
{
        if (!cache->empty) return KPAGE_ERR;

        struct kslab *slab = cache->empty;
        slab_relink_node(&cache->empty, &cache->partial, slab);
        cache->empty_slabs--;

        void **retalloc = (void **)slab->freelist;
        slab->freelist = *retalloc;
        slab->free_count--;


        return (void *)retalloc;
}
static void *slab_new_alloc(struct kslab_cache *cache)
{
        struct kslab *slab = new_slab_init(cache);


        if (unlikely(slab == KPAGE_ERR)) {
                return KPAGE_ERR;
        }

        return slab_empty_alloc(cache);
}

// priority: partial -> empty -> new
static void *(*slab_alloc_type[])(struct kslab_cache *cache) = {
        &slab_partial_alloc,
        &slab_empty_alloc,
        &slab_new_alloc,
};




void slab_free(struct kslab_cache *cache, void *obj)
{
        uintptr_t pv = (uintptr_t)obj;
        uintptr_t base = pv & ~(buddy_order_to_val(cache->order) - 1);
        struct kslab *slab = (void *)base;

        const bool was_full = (slab->free_count == 0);

        *(void **)obj = slab->freelist;
        slab->freelist = obj;
        slab->free_count++;

        const bool is_empty = (slab->free_count == cache->slab_objs_per);

        if (was_full) {
                slab_relink_node(&cache->full, &cache->partial, slab);
        } else if (is_empty) {
                slab_relink_node(&cache->partial, &cache->empty, slab);
                cache->empty_slabs++;

                if (++cache->empty_slabs > cache->max_empty_slabs) {
                        slab_remove_node(cache, &cache->empty, cache->empty);
                        cache->empty_slabs--;
                }
        }
}




_hot_ _malloc_arg_(slab_free, 2)
void *slab_alloc(struct kslab_cache *cache)
{
        if (unlikely(!cache)) return KPAGE_ERR;
        void *obj = KPAGE_ERR;

        for (usize i = 0; i < sizeof(slab_alloc_type) / sizeof(slab_alloc_type[0]); i++) {
                obj = (slab_alloc_type[i])(cache);
                if (obj != KPAGE_ERR) break;
        }

        return obj;
}

