// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "buddy.h"

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




/* Call find_valid_slave() instead. this only checks preceeding. */
static slave_buddy_t *find_buddy_slave(const phys_addr_t phy)
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

                if (virt_to_phys_pm(MASTER_BUDDY.regions[mid]) <= phy) {
                        owner = mid;
                        lo = mid + 1;
                } else {
                        hi = mid - 1;
                }
        }

        return owner != -1 ? MASTER_BUDDY.regions[owner] : KPAGE_ERR;
}




static slave_buddy_t *find_valid_slave(const phys_addr_t phy)
{
        slave_buddy_t *s = find_buddy_slave(phy);

        if (s == KPAGE_ERR || phy >= s->mbase + (s->total_pages * PAGESIZE)) {
                return KPAGE_ERR;
        }

        return s;
}




_cold_
static inline void kbud_mark_invalid(slave_buddy_t *virt_region, phys_addr_t phy)
{
        memset(virt_region, (u8)-1, sizeof(*virt_region));
        virt_region->rbase = phy;
        virt_region->total_pages = 0;
}




_cold_
// static void kbud_init_region(slave_buddy_t *region, const usize len)
static void kbud_init_region(phys_addr_t phy, const usize len)
{
        /* It is 64 bytes to allow the compiler to vectorize.           *
         * The bigger this number is, the more waste bytes there is.    */
        static constexpr usize BITMAP_ALIGN = 0x40;

        slave_buddy_t *virt_region = (void *)phys_to_virt_pm(phy);

        const usize rpages = PAGE_ALIGNDOWN(len) >> 12;
        virt_region->total_pages = rpages;
        virt_region->rbase = phy;

        usize total_bytes = 0;
        /* cur is used as an aligned bitmap base offset, and it can be  *
         * used for the end of the last bitmap too for setting mbase.   */
        u8 *virt_cur = (u8 *)((((uintptr_t)virt_region + sizeof(*virt_region)) + 7) & ~(usize)7);

        for (usize i = 0; i < PFRAME_MAX_ORDER; i++) {
                const usize bits = rpages >> (i + 1);
                if (unlikely(!bits)) break;

                const usize bytes = (bits + 7) >> (usize)3;

                virt_region->lut.bitmap_arr[i] = virt_cur;
                virt_region->lut.maxbits[i] = bits;
                virt_region->max_order = (bud_order_t)i;
                total_bytes += SY_ALIGN_UP(bytes, BITMAP_ALIGN);
                virt_cur += SY_ALIGN_UP(bytes, BITMAP_ALIGN);
        }


        // virt_region->lut.base = (phys_addr_t)((u8 *)virt_region + sizeof(*virt_region));
        virt_region->lut.base = phy + sizeof(*virt_region);
        phys_addr_t phys_cur = phy + ((u8 *)virt_cur - (u8 *)virt_region);
        virt_region->mbase = PAGE_ALIGNUP(phys_cur);

        /* if mbase is ever equal or greater than               *
         * region + len, that means the tiny bitmap             *
         * alone swallowed all the space, its a useless region. */
        if (unlikely(virt_region->mbase >= phy + len)) {
                goto err;
        }

        const phys_addr_t phys_end = phy + PAGESIZE;
        virt_region->lut.bitmap_reserved = (virt_region->mbase > phys_end)
                ? (virt_region->mbase - phys_end) >> 12
                : 0;

        virt_region->total_pages = (phy + len - virt_region->mbase) >> 12;
        usize hdr_offs = (u8 *)virt_cur - (u8 *)virt_region;
        memset((u8 *)virt_region + sizeof(*virt_region), 0, hdr_offs - sizeof(*virt_region));

        return;
err:
        kbud_mark_invalid(virt_region, phy);
}




slave_buddy_t *kbud_add_region(const phys_addr_t phy, const usize len)
{
        if (unlikely(MASTER_BUDDY.region_count + 1 > PFRAME_MAX_REGIONS)) {
                return KPAGE_ERR;
        }

        usize *rc = &MASTER_BUDDY.region_count;
        slave_buddy_t **mbr = MASTER_BUDDY.regions;

        usize i = *rc;
        slave_buddy_t *virt_region = (void *)phys_to_virt_pm(phy);

        while (i > 0 && mbr[i - 1] > virt_region) {
                mbr[i] = mbr[i - 1];
                i--;
        }

        mbr[i] = virt_region;
        mbr[++(*rc)] = KPAGE_ERR;

        memset(virt_region, 0, sizeof(*virt_region));
        kbud_init_region(phy, len);

        return virt_region;
}




_const_
static inline u8 kbud_byte_order(usize sz)
{
        const usize frames = (sz + PAGESIZE - 1) >> 12;
        if (frames <= 1) return 0;
        return (u8)(63 - count_leading_zeroes(frames - 1)) + 1;
}


_reproducible_
static inline phys_addr_t kbud_pfnum(const phys_addr_t phy, slave_buddy_t *s)
{
        return (phy - s->mbase) >> 12;
}

_const_
static inline usize kbud_pair_idx(const pfn_t pfn, bud_order_t ord)
{
        return (usize)(pfn >> (ord + 1));
}


/* checks for a valid bit from pfn in slave, returns false if not valid */
_reproducible_ _nonnull_(1)
static inline bool buddy_bit_chk(slave_buddy_t *s, pfn_t pfn, bud_order_t ord)
{
        if (ord > s->max_order) return false;
        const usize pidx = kbud_pair_idx(pfn, ord);
        if (pidx >= s->lut.maxbits[ord]) return false;

        return true;
}

_reproducible_ _nonnull_(1)
static inline bool buddy_bit_get(slave_buddy_t *s, pfn_t pfn, bud_order_t ord) 
{
        const usize pidx = kbud_pair_idx(pfn, ord);

        ASSERT(ord <= s->max_order || pidx < s->lut.maxbits[ord], 
               "attempt to access invalid bit in kbuddy!");

        return (s->lut.bitmap_arr[ord][pidx >> 3] >> (pidx & 7)) & 1;
}

_nonnull_(1)
static inline bool buddy_bit_set(slave_buddy_t *s, pfn_t pfn, bud_order_t ord)
{
        const usize pidx = kbud_pair_idx(pfn, ord);

        ASSERT(ord <= s->max_order || pidx < s->lut.maxbits[ord], 
               "attempt to mutate invalid bit in kbuddy!");

        return (s->lut.bitmap_arr[ord][pidx >> 3] |= (u8)(1 << (pidx & 7)));
}

_nonnull_(1)
static inline bool buddy_bit_clr(slave_buddy_t *s, pfn_t pfn, bud_order_t ord)
{
        const usize pidx = kbud_pair_idx(pfn, ord);

        ASSERT(ord <= s->max_order || pidx < s->lut.maxbits[ord], 
               "attempt to mutate invalid bit in kbuddy!");

        return (s->lut.bitmap_arr[ord][pidx >> 3] &= (u8)~(1 << (pidx & 7)));
}




/* returns true if push was successful, false on error. */
static bool kbud_push(slave_buddy_t *slave, phys_addr_t phy, bud_order_t order)
{
        pfn_t pfn = kbud_pfnum(phy, slave);
        if (unlikely(!buddy_bit_chk(slave, pfn, order))) {
                return false;
        }

        page_frame_t *p = phys_to_virt_pm(phy);

        p->order = order;
        p->next = slave->free[order];
        slave->free[order] = p;
        slave->order_blk_cnt[order]++;

        buddy_bit_set(slave, pfn, order);

        return true;
}

/* returns true if pop was successful, false on error. */
static bool kbud_pop(slave_buddy_t *slave, phys_addr_t phy, bud_order_t order)
{
        pfn_t pfn = kbud_pfnum(phy, slave);

        if (unlikely(!buddy_bit_chk(slave, pfn, order) 
                                || unlikely(slave->order_blk_cnt[order] == 0))) {
                return false;
        }


        /* the linus torvalds algorithm :p */
        page_frame_t **cur = &slave->free[order];
        page_frame_t *p = phys_to_virt_pm(phy);

        while (*cur != p) {
                cur = &(*cur)->next;
        }

        ASSERT(*cur != NULL, "buddy list corruption detected! free addr not found in free list");

        *cur = (*cur)->next;
        slave->order_blk_cnt[order]--;

        buddy_bit_clr(slave, pfn, order);

        return true;
}

/* returns valid ptr if pop was successful, KPAGE_ERR on error. */
static void *kbud_pop_head(slave_buddy_t *slave, bud_order_t order)
{
        page_frame_t *blk = slave->free[order];

        pfn_t pfn = kbud_pfnum(virt_to_phys_pm(blk), slave);
        if (unlikely(!buddy_bit_chk(slave, pfn, order))) {
                return KPAGE_ERR;
        }


        slave->free[order] = blk->next;
        slave->order_blk_cnt[order]--;

        buddy_bit_clr(slave, pfn, order);
        
        return blk;
}




/* prefer using kbud_free() instead, and track the order.  */
_hot_
void kbud_free_range(void *vaddr, const usize len)
{
        phys_addr_t phys = PAGE_ALIGNDOWN(virt_to_phys_pm(vaddr));

        slave_buddy_t *s = find_valid_slave(phys);
        usize size       = PAGE_ALIGNDOWN(len);

        if (unlikely(s == KPAGE_ERR)) {
                s = kbud_add_region(phys, len);
                if (unlikely(s == KPAGE_ERR)) return;

                const phys_addr_t offs = ((s->mbase - s->rbase) + (PAGESIZE - 1)) & ~(PAGESIZE - 1);
                phys += offs;
                size -= offs;
        }

        static constexpr bud_order_t max_order = PFRAME_MAX_ORDER - 1;

        while (size > 0) {
                u8 len_order = (u8)(63 - count_leading_zeroes(size / PAGESIZE));
                u8 ptr_order = (u8)kbud_order_idx(phys);

                u8 ord = MIN(len_order, ptr_order);
                ord = MIN(max_order, ord);
                ord = MIN(s->max_order, ord);

                kbud_push(s, phys, ord);

                const usize blksz = (usize)PAGESIZE << ord;
                phys += blksz;
                size -= blksz;
        }
}




/* allocates at most size PFRAME_MAX_ORDER - 1, returns KPAGE_ERR on error.     *
 * prefer using kbud_alloc() instead, and track the order.                      */
_hot_ _malloc_
void *kbud_alloc_range(usize sz)
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

                if (unlikely(addr == KPAGE_ERR)) {
                        continue;
                }

                while (order > req) {
                        order--;
                        phys_addr_t buddy = virt_to_phys_pm(addr) ^ ((phys_addr_t)PAGESIZE << order);
                        kbud_push(s, buddy, order);
                }

                return addr;
        }

        return KPAGE_ERR;
}




_hot_ _malloc_
void *kbud_alloc(const bud_order_t req)
{
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
                        order--;
                        phys_addr_t buddy = virt_to_phys_pm(addr) ^ ((phys_addr_t)PAGESIZE << order);
                        kbud_push(s, buddy, order);
                }

                return addr;
        }

        return KPAGE_ERR;
}




_hot_
void kbud_free(void *vaddr, const bud_order_t order)
{
        const phys_addr_t phy = virt_to_phys_pm(vaddr);
        slave_buddy_t *s = find_valid_slave(phy);

        if (unlikely(s == KPAGE_ERR)) 
                return;

        pfn_t pfn = kbud_pfnum(phy, s);
        bud_order_t k = order;

        while (k < PFRAME_MAX_ORDER - 1) {
                pfn_t bpfn = pfn ^ (1ULL << k);

                if (bpfn >= s->total_pages) break;
                if (!buddy_bit_chk(s, bpfn, k)) break;
                if (!buddy_bit_get(s, bpfn, k)) break;

                phys_addr_t buddy = s->mbase + (bpfn << 12);

                kbud_pop(s, buddy, k);
                buddy_bit_clr(s, bpfn, k);

                pfn &= ~(1ULL << k);
                k++;
        }

        kbud_push(s, s->mbase + (pfn << 12), k);
}

