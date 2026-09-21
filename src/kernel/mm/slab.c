// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "slab.h"
#include "types.h"

#define SLAB_SIZE_PER   0x400000
#define SLAB_SIZE(i)    (1ULL << slab_shift[i])
#define SLAB_COUNT      (sizeof(slab_shift) / sizeof(slab_shift[0]))
#define SLAB_MAX_SZ     ((SLAB_COUNT) * (SLAB_SIZE_PER))

static_assert((SLAB_SIZE_PER % 0x4000) == 0, "invalid slab sizes!");

#define SLAB_SET \
        X(0, 16) \
        X(1, 32) \
        X(2, 64) \
        X(3, 128) \
        X(4, 256) \
        X(5, 512) \
        X(6, 1024) \
        X(7, 2048)
#define X(id, v)        __builtin_ctzll(v),

static constexpr u32 slab_shift[] = {SLAB_SET};
#undef X
static constexpr u32 slab_sz_shift = __builtin_ctzll(SLAB_SIZE_PER);

struct kslab_meta {
        u64 bitmap[16];
        byte *base;

        u32 elemsz;
        u32 elems;
};

struct kslab {
        struct kslab_meta blocks[SLAB_COUNT];
};

#define X(id, v) \
        {{}, NULL, 1U << slab_shift[id], SLAB_SIZE_PER >> slab_shift[id]},
static struct kslab slab = {
        .blocks = {SLAB_SET},
};
#undef X

#define SLAB_ALLOC_MAX_SZ slab.blocks[SLAB_COUNT - 1].elemsz
#define SLAB_ALLOC_MIN_SZ slab.blocks[0].elemsz




void kmalloc_slab_init(void)
{
}

