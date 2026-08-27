// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "macros.h"
#include "types.h"
#include "pagetbl.h"
#include "builtin.h"

#define PFRAME_MAX_REGIONS      128
#define PFRAME_MAX_ORDER        11

typedef uintptr_t phys_addr_t;

typedef enum buddy_page_order {
        BUDDY_ORDER_4K = 0,
        BUDDY_ORDER_8K = 1,
        BUDDY_ORDER_16K = 2,
        BUDDY_ORDER_32K = 3,
        BUDDY_ORDER_64K = 4,
        BUDDY_ORDER_128K = 5,
        BUDDY_ORDER_256K = 6,
        BUDDY_ORDER_512K = 7,
        BUDDY_ORDER_1024K = 8,
        BUDDY_ORDER_2048K = 9,
        BUDDY_ORDER_4096K = 10,


        BUDDY_ORDER_1M = 8,
        BUDDY_ORDER_2M = 9,
        BUDDY_ORDER_4M = 10,
} bud_order_t;


/* this is metadata, but it isn't just metadata.
 * making it pagealigned makes it easy to do whole-page 
 * pointer arithmetic. it's basically a representation of a whole page. */
typedef struct _kpage_frame {
        struct _kpage_frame *next;

        /* future someone: as long as order can go upto MAX_ORDER,      *
         * its fine to shorten this and add bitfields.                  */
        u8 order;
} _PAGEALIGNED page_frame_t;


/* also known as the region header. this has approximately 3896 bytes of extra  *
 * usable space at the time of writing this comment, for future expansion ----- */
typedef struct _kslave_buddy {
        page_frame_t *free[PFRAME_MAX_ORDER];
        u64 order_blk_cnt[PFRAME_MAX_ORDER];
        bud_order_t max_order;

        struct {
                u8 *bitmap_arr[PFRAME_MAX_ORDER];
                usize maxbits[PFRAME_MAX_ORDER];
                usize bitmap_reserved;
                phys_addr_t base;

        } lut;

        phys_addr_t rbase;      /**< base of the whole region --------- */
        phys_addr_t mbase;      /**< base of the usable region memory - */
        usize total_pages;
} slave_buddy_t;

static_assert(sizeof(page_frame_t) == PAGESIZE, "page_frame_t not equal to size of page!\n");
static_assert(sizeof(slave_buddy_t) <= PAGESIZE, "slave_buddy_t larger than size of page!\n");

/* for future someone who may want to check how much    *
 * space is left via code hover LSP or similar.         */
static constexpr i64 KPFRAME_REMAINING_SLAVE_SZ = PAGESIZE - sizeof(slave_buddy_t);


/* sadly, master_buddy has no buddies. what a loser. */
typedef struct _kmaster_buddy {
        slave_buddy_t *regions[PFRAME_MAX_REGIONS];

        usize region_count;
        // total ram includes holes like NVS, BADRAM and reserved. (should this even exist?)
        usize total_ram;
        // usable ram is self explanatory, only ram regions marked available.
        usize usable_ram;
} master_buddy_t;


extern master_buddy_t MASTER_BUDDY;


void *kbud_allocate(usize sz);
void kbud_free(void *addr, bud_order_t order);
extern void kbud_free_range(const phys_addr_t phys, const usize len);

