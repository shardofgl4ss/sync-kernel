// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "macros.h"


#define PAGESIZE                4096
#define _PAGEALIGNED            __attribute__((aligned(PAGESIZE)))
#define PAGE_ALIGNUP(x)         (((x) + (PAGESIZE - 1)) & ~(PAGESIZE - 1))
#define PAGE_ALIGNDOWN(x)       ((x) & ~(PAGESIZE - 1))


typedef struct x86_page_tbl {
        u64 entries[512];
} _PAGEALIGNED PageTable;

static inline void tlb_invl_page(u64 vaddr)
{
        __asm__ volatile (
                "invlpg (%0)"
                :
                : "r"(vaddr)
                : "memory"
        );
}

static inline void tlb_flush(void)
{
	u64 cr3;
        __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
        __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

typedef struct kslab_cache kslab_cache_t;

/* this is metadata, but it isn't just metadata.
 * making it pagealigned makes it easy to do whole-page 
 * pointer arithmetic. it's basically a representation of a whole page. */
typedef struct _kpage_frame {
        struct _kpage_frame *next;
        kslab_cache_t *cache;

        /* future someone: as long as order can go upto MAX_ORDER,      *
         * its fine to shorten this and add bitfields.                  */
        u8 order;
} _PAGEALIGNED page_frame_t;


static_assert(sizeof(PageTable) == 0x1000, __FILE__ " PageTable size error\n");

/* Returns a physmap ptr to a valid page table if it exists. If not, this allocates one and returns it. */
extern PageTable *return_pt_or_alloc(u64 *const restrict pte);

_const_ _always_inline_
static inline u64 page_l4_idx(void *virt) { return (((u64)virt >> 39) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_l3_idx(void *virt) { return (((u64)virt >> 30) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_l2_idx(void *virt) { return (((u64)virt >> 21) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_l1_idx(void *virt) { return (((u64)virt >> 12) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_offs_idx(void *virt) { return ((u64)virt & 0xFFF); }
_const_ _always_inline_
static inline phys_addr_t page_phys_rebase(phys_addr_t phys, u64 shift, u64 mask)
{
        return ((phys >> shift) & mask) << shift;
}



#define BITRANGE_MASK(a, b)     ((1ULL << ((a) - (b) + 1ULL)) - 1ULL)
#define PT_REBASE(p)            (page_phys_rebase((phys_addr_t)(p), 12, BITRANGE_MASK(51, 12)))


static constexpr usize PT_ENTRIES = 512;


static constexpr usize PT_L1_ENTRYSZ = sizeof(PageTable);
static constexpr usize PT_L2_ENTRYSZ = PT_L1_ENTRYSZ * PT_ENTRIES;
static constexpr usize PT_L3_ENTRYSZ = PT_L2_ENTRYSZ * PT_ENTRIES;
static constexpr usize PT_L4_ENTRYSZ = PT_L3_ENTRYSZ * PT_ENTRIES;

static constexpr usize PT_L2_ENTRYMASK = PT_L2_ENTRYSZ - 1;
static constexpr usize PT_L3_ENTRYMASK = PT_L3_ENTRYSZ - 1;
static constexpr usize PT_L4_ENTRYMASK = PT_L4_ENTRYSZ - 1;


_const_ static inline usize get_pagetbl_count_l3(const usize bytes)
{
        return ((bytes + PT_L4_ENTRYMASK) & ~PT_L4_ENTRYMASK) / PT_L4_ENTRYSZ;
}

_const_ static inline usize get_pagetbl_count_l2(const usize bytes)
{
        return ((bytes + PT_L3_ENTRYMASK) & ~PT_L3_ENTRYMASK) / PT_L3_ENTRYSZ;
}

_const_ static inline usize get_pagetbl_count_l1(const usize bytes)
{
        return ((bytes + PT_L2_ENTRYMASK) & ~PT_L2_ENTRYMASK) / PT_L2_ENTRYSZ;
}


extern char KERNEL_OFFSET[];
extern char KERNEL_VMMMAP[];
extern char KERNEL_PHMMAP[];

#define KPAGE_ERR               ((void *)-1)


typedef enum {
	PT_FLAG_PRESENT                 = 1 << 0,
	PT_FLAG_WRITABLE                = 1 << 1,
	PT_FLAG_USER                    = 1 << 2,
	PT_FLAG_WRITE_THROUGH           = 1 << 3,
	PT_FLAG_PAGECACHE_DISABLE       = 1 << 4,
	PT_FLAG_DIRTY                   = 1 << 5,
        PT_FLAG_PAGESIZE                = 1 << 7,
} PT_FLAG_BITS;


enum addrspace_alloc_type {
	ADDRSPACE_ALLOC_TYPE_4K = 0,
	// 2MiB pages.
	ADDRSPACE_ALLOC_TYPE_2M = 1,
	// 1GiB pages.
	ADDRSPACE_ALLOC_TYPE_1G = 2,
};




// /* maps a region of size bytes,                 *
//  * returns KPAGE_ERR on error,                  *
//  * or a valid ptr to address space on success.  *
//  * phys is rounded down to nearest page size,   *
//  * bytes is rounded up to multiple of pagesize. *
//  * zero is a valid type for standard pages.     */
// extern void *kmap(void *phys, void *virt, u64 bytes, enum addrspace_alloc_type type);


// so the compiler will fucking stop generating 32 bit relocs to 64 bit absolute symbols.
/* fuck you, compiler. */
_const_ _always_inline_ 
static inline u64 get_kernel_phmap(void)
{
        u64 x;
        __asm__ volatile ("movabs $KERNEL_PHMMAP, %0" : "=r"(x));
        return x;
}
/* fuck you, compiler. */
_const_ _always_inline_ 
static inline u64 get_kernel_offs(void)
{
        u64 x;
        __asm__ volatile ("movabs $KERNEL_OFFSET, %0" : "=r"(x));
        return x;
}
/* fuck you, compiler. */
_const_ _always_inline_ 
static inline u64 get_kernel_vmmap(void)
{
        u64 x;
        __asm__ volatile ("movabs $KERNEL_VMMMAP, %0" : "=r"(x));
        return x;
}


extern PageTable *kpt_get_l4(void);
extern PageTable *kpt_get_l3(void *vaddr, PageTable *_Nullable l4);
extern PageTable *kpt_get_l2(void *vaddr, PageTable *_Nullable l3);
extern PageTable *kpt_get_l1(void *vaddr, PageTable *_Nullable l2);


/* linear mappings only. */

/* phys/virt conversion of linear ram map (at 0xFFFF800000000000) */
_const_ _always_inline_
static inline phys_addr_t virt_to_phys_pm(void *v) { return (phys_addr_t)((u64)v - get_kernel_phmap()); }
/* phys/virt conversion of linear ram map (at 0xFFFF800000000000) */
_const_ _always_inline_
static inline void *phys_to_virt_pm(phys_addr_t p) { return (void *)(p + get_kernel_phmap()); }
/* phys/virt conversion of linear kernel code map (at 0xFFFFFFFF80000000) */
_const_ _always_inline_
static inline phys_addr_t virt_to_phys_of(void *v) { return (phys_addr_t)((u64)v - get_kernel_offs()); }
/* phys/virt conversion of linear kernel code map (at 0xFFFFFFFF80000000) */
_const_ _always_inline_
static inline void *phys_to_virt_of(phys_addr_t p) { return (void *)(p + get_kernel_offs()); }

/* probably don't need virt/phys conversion for io. */



extern int kern_uhalf_init(void);

