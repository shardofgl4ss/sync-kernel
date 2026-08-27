// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "pframe.h"

#include "boot.h"
#include "kstring.h"
#include "macros.h"
#include "multiboot2.h"
#include "pagetbl.h"
#include "panic.h"
#include "types.h"

#include "builtin.h"

extern char _kphys_start[];
extern char _kphys_end[];

master_buddy_t MASTER_BUDDY;

#define ASSERT_FUNC(str)     panic(str)

#ifdef DEBUG
#define ASSERT(expr, str) \
        do { \
                if (unlikely(!(expr))) { \
                        ASSERT_FUNC(str); \
                } \
        } while (0)
#else
#define ASSERT(expr, str)

#endif

#define MIN(a, b)       ((a) < (b)) ? (a) : (b)


_const_ _always_inline_
static inline usize kbud_order_idx(usize n)
{
        return count_trailing_zeroes(n) - 12;
}




/* Call check_valid_sbuddy() instead during init. this only checks preceeding. */
static slave_buddy_t *find_buddy_slave(const phys_addr_t addr)
{
        if (unlikely(MASTER_BUDDY.region_count == 0 
            || MASTER_BUDDY.region_count > PFRAME_MAX_REGIONS)) {
                return KPAGE_ERR;
        }

        isize lo = 0;
        isize hi = MASTER_BUDDY.region_count - 1;
        isize owner = -1;

        while (lo <= hi) {
                isize mid = (lo + hi) / 2;

                if ((phys_addr_t)MASTER_BUDDY.regions[mid] <= addr) {
                        owner = mid;
                        lo = mid + 1;
                } else {
                        hi = mid - 1;
                }
        }

        return owner != -1 ? MASTER_BUDDY.regions[owner] : KPAGE_ERR;
}




static slave_buddy_t *find_valid_slave(const phys_addr_t addr)
{
        slave_buddy_t *s = find_buddy_slave(addr);

        if (s == KPAGE_ERR || addr >= s->mbase + (s->total_pages * PAGESIZE)) {
                return KPAGE_ERR;
        }

        return s;
}




_cold_
static inline void kbud_mark_invalid(slave_buddy_t *region)
{
        memset(region, (u8)-1, sizeof(*region));
        region->rbase = (phys_addr_t)region;
        region->total_pages = 0;
}




_cold_
static void kbud_init_region(slave_buddy_t *region, const usize len)
{
        /* It is 64 bytes to allow the compiler to vectorize.           *
         * The bigger this number is, the more waste bytes there is.    */
        static constexpr usize BITMAP_ALIGN = 0x40;

        const usize rpages = PAGE_ALIGNDOWN(len) >> 12;
        region->total_pages = rpages;
        region->rbase = (phys_addr_t)region;

        usize total_bytes = 0;
        /* cur is used as an aligned bitmap base offset, and it can be  *
         * used for the end of the last bitmap too for setting mbase.   */
        u8 *cur = (u8 *)(((region->rbase + sizeof(*region)) + 7) & ~(usize)7);

        for (usize i = 0; i < PFRAME_MAX_ORDER; i++) {
                const usize bits = rpages >> (i + 1);
                if (unlikely(!bits)) break;

                const usize bytes = (bits + 7) >> (usize)3;

                region->lut.bitmap_arr[i] = cur;
                region->lut.maxbits[i] = bits;
                region->max_order = (bud_order_t)i;
                total_bytes += SY_ALIGN_UP(bytes, BITMAP_ALIGN);
                cur += SY_ALIGN_UP(bytes, BITMAP_ALIGN);
        }


        region->lut.base = (phys_addr_t)((u8 *)region + sizeof(*region));
        region->mbase = PAGE_ALIGNUP((phys_addr_t)cur);

        /* if mbase is ever equal or greater than               *
         * region + len, that means the tiny bitmap             *
         * alone swallowed all the space, its a useless region. */
        if (unlikely(region->mbase >= (phys_addr_t)region + len)) {
                goto err;
        }

        const phys_addr_t end = region->rbase + PAGESIZE;
        region->lut.bitmap_reserved = (region->mbase > end)
                ? (region->mbase - end) >> 12
                : 0;

        region->total_pages = ((phys_addr_t)region + len - region->mbase) >> 12;
        memset((u8 *)region + sizeof(*region), 0, (phys_addr_t)cur - (phys_addr_t)region);

        return;
err:
        kbud_mark_invalid(region);
}




slave_buddy_t *kbud_add_region(const phys_addr_t addr, const usize len)
{
        if (unlikely(MASTER_BUDDY.region_count + 1 > PFRAME_MAX_REGIONS)) {
                return KPAGE_ERR;
        }

        usize *rc = &MASTER_BUDDY.region_count;
        slave_buddy_t **mbr = MASTER_BUDDY.regions;

        usize i = *rc;
        while (i > 0 && mbr[i - 1] > (slave_buddy_t *)addr) {
                mbr[i] = mbr[i - 1];
                i--;
        }

        mbr[i] = (slave_buddy_t *)addr;
        mbr[++(*rc)] = KPAGE_ERR;

        memset(mbr[i], 0, sizeof(*mbr[i]));
        kbud_init_region(mbr[i], len);

        return mbr[i];
}




_const_
static inline u8 kbud_byte_order(usize sz)
{
        const usize frames = (sz + PAGESIZE - 1) >> 12;
        if (frames <= 1) return 0;
        return (u8)(63 - count_leading_zeroes(frames - 1)) + 1;
}


_reproducible_
static inline phys_addr_t kbud_pfnum(const phys_addr_t addr, slave_buddy_t *s)
{
        return (addr - s->mbase) >> 12;
}

_const_
static inline usize kbud_pair_idx(const phys_addr_t pfn, bud_order_t ord)
{
        return (usize)(pfn >> (ord + 1));
}


/* checks for a valid bit from pfn in slave, returns false if not valid */
_reproducible_ _nonnull_(1)
static inline bool kbuddy_bit_chk(slave_buddy_t *s, phys_addr_t pfn, bud_order_t ord)
{
        if (ord > s->max_order) return false;
        const usize pidx = kbud_pair_idx(pfn, ord);
        if (pidx >= s->lut.maxbits[ord]) return false;

        return true;
}

_reproducible_ _nonnull_(1)
static inline bool kbuddy_bit_get(slave_buddy_t *s, phys_addr_t pfn, bud_order_t ord) 
{
        const usize pidx = kbud_pair_idx(pfn, ord);

        ASSERT(ord > s->max_order || pidx >= s->lut.maxbits[ord], 
               "attempt to access invalid bit in kbuddy!");

        return (s->lut.bitmap_arr[ord][pidx >> 3] >> (pidx & 7)) & 1;
}

_nonnull_(1)
static inline bool kbuddy_bit_set(slave_buddy_t *s, phys_addr_t pfn, bud_order_t ord)
{
        const usize pidx = kbud_pair_idx(pfn, ord);

        ASSERT(ord > s->max_order || pidx >= s->lut.maxbits[ord], 
               "attempt to mutate invalid bit in kbuddy!");

        return (s->lut.bitmap_arr[ord][pidx >> 3] |= (u8)(1 << (pidx & 7)));
}

_nonnull_(1)
static inline bool kbuddy_bit_clr(slave_buddy_t *s, phys_addr_t pfn, bud_order_t ord)
{
        const usize pidx = kbud_pair_idx(pfn, ord);

        ASSERT(ord > s->max_order || pidx >= s->lut.maxbits[ord], 
               "attempt to mutate invalid bit in kbuddy!");

        return (s->lut.bitmap_arr[ord][pidx >> 3] &= (u8)~(1 << (pidx & 7)));
}




/* returns true if push was successful, false on error. */
static bool kbud_push(slave_buddy_t *slave, void *addr, bud_order_t order)
{
        phys_addr_t pfn = kbud_pfnum((phys_addr_t)addr, slave);
        if (unlikely(!kbuddy_bit_chk(slave, pfn, order))) {
                return false;
        }

        page_frame_t *p = addr;

        p->order = order;
        p->next = slave->free[order];
        slave->free[order] = p;
        slave->order_blk_cnt[order]++;

        kbuddy_bit_set(slave, pfn, order);

        return true;
}

/* returns true if pop was successful, false on error. */
static bool kbud_pop(slave_buddy_t *slave, void *addr, bud_order_t order)
{
        phys_addr_t pfn = kbud_pfnum((phys_addr_t)addr, slave);
        if (unlikely(!kbuddy_bit_chk(slave, pfn, order) 
         || unlikely(slave->order_blk_cnt[order] == 0))) {
                return false;
        }

        /* the linus torvalds algorithm :p */
        page_frame_t **cur = &slave->free[order];

        while (*cur != (page_frame_t *)addr) {
                cur = &(*cur)->next;
        }

        *cur = (*cur)->next;
        slave->order_blk_cnt[order]--;

        kbuddy_bit_clr(slave, pfn, order);

        return true;
}

/* returns valid ptr if pop was successful, KPAGE_ERR on error. */
static void *kbud_pop_head(slave_buddy_t *slave, bud_order_t order)
{
        page_frame_t *blk = slave->free[order];

        phys_addr_t pfn = kbud_pfnum((phys_addr_t)blk, slave);
        if (unlikely(!kbuddy_bit_chk(slave, pfn, order))) {
                return KPAGE_ERR;
        }


        slave->free[order] = blk->next;
        slave->order_blk_cnt[order]--;

        kbuddy_bit_clr(slave, pfn, order);
        
        return blk;
}




void kbud_free_range(const phys_addr_t phys, const usize len)
{
        phys_addr_t addr = PAGE_ALIGNDOWN((phys_addr_t)phys);

        slave_buddy_t *s = find_valid_slave(phys);
        usize size       = PAGE_ALIGNDOWN(len);

        if (unlikely(s == KPAGE_ERR)) {
                s = kbud_add_region(addr, len);
                if (unlikely(s == KPAGE_ERR)) return;

                phys_addr_t offs = ((s->mbase - s->rbase) + (PAGESIZE - 1)) & ~(PAGESIZE - 1);
                addr += offs;
                size -= offs;
        }

        static constexpr bud_order_t max_order = PFRAME_MAX_ORDER - 1;

        while (size > 0) {
                u8 len_order = (u8)(63 - count_leading_zeroes(size / PAGESIZE));
                u8 ptr_order = (u8)kbud_order_idx((usize)addr);

                u8 ord = MIN(len_order, ptr_order);
                ord = MIN(max_order, ord);
                ord = MIN(s->max_order, ord);

                kbud_push(s, (void *)addr, ord);

                const usize blksz = (usize)PAGESIZE << ord;
                addr += blksz;
                size -= blksz;
        }
}




/* allocates at most size PFRAME_MAX_ORDER - 1, returns KPAGE_ERR on error. */
_hot_
void *kbud_allocate(usize sz)
{
        const usize rounded_sz = PAGE_ALIGNUP(sz);
        const u8 req = kbud_byte_order(rounded_sz);

        if (req >= PFRAME_MAX_ORDER) {
                return KPAGE_ERR;
        }

        for (usize i = 0; i < MASTER_BUDDY.region_count; i++) {
                slave_buddy_t *s = MASTER_BUDDY.regions[i];
                
                if (unlikely(s == KPAGE_ERR)) {
                        continue;
                }

                u8 order = req;
                while (order < PFRAME_MAX_ORDER 
                                && s->order_blk_cnt[order] == 0 
                                && order < s->max_order) {
                        order++;
                }

                if (order == PFRAME_MAX_ORDER || order < req) {
                        continue;
                }

                void *addr = kbud_pop_head(s, order);

                if (unlikely(addr == KPAGE_ERR)) continue;

                while (order > req) {
                        const phys_addr_t order_shift = (phys_addr_t)PAGESIZE << order++;
                        void *buddy = (void *)((phys_addr_t)addr ^ order_shift);

                        kbud_push(s, buddy, order);
                }

                return addr;
        }

        return KPAGE_ERR;
}




_hot_
void kbud_free(void *addr, bud_order_t order)
{
        slave_buddy_t *s = find_valid_slave((phys_addr_t)addr);

        if (unlikely(s == KPAGE_ERR)) 
                return;

        phys_addr_t pfn = kbud_pfnum((phys_addr_t)addr, s);
        bud_order_t k = order;

        while (k < PFRAME_MAX_ORDER - 1) {
                phys_addr_t bpfn = pfn ^ (1ULL << k);

                if (bpfn >= s->total_pages) break;
                if (!kbuddy_bit_chk(s, bpfn, k)) break;
                if (!kbuddy_bit_get(s, bpfn, k)) break;

                void *buddy = (void *)(s->mbase + (bpfn << 12));

                kbud_pop(s, buddy, k);
                kbuddy_bit_clr(s, bpfn, k);

                pfn &= ~(1ULL << k);
                k++;
        }

        kbud_push(s, (void *)(s->mbase + (pfn << 12)), k);
}

