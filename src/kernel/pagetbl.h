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

static_assert(sizeof(PageTable) == 0x1000, __FILE__ " PageTable size error\n");


_const_ _always_inline_
static inline u64 page_l4_idx(void *vaddr) { return (((u64)vaddr >> 39) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_l3_idx(void *vaddr) { return (((u64)vaddr >> 30) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_l2_idx(void *vaddr) { return (((u64)vaddr >> 21) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_l1_idx(void *vaddr) { return (((u64)vaddr >> 12) & 0x1FF); }
_const_ _always_inline_
static inline u64 page_offs_idx(void *vaddr) { return ((u64)vaddr & 0xFFF); }
_const_ _always_inline_
static inline void *page_phys_rebase(void *paddr, u64 shift, u64 mask)
{
        return (void *)((((u64)paddr >> shift) & mask) << shift);
}



#define BITRANGE_MASK(a, b)     ((1ULL << ((a) - (b) + 1ULL)) - 1ULL)
#define PT_REBASE(p)            (void *)page_phys_rebase( \
                (void *)(p), \
                12, \
                BITRANGE_MASK(51, 12))



extern char                     KERNEL_OFFSET[];
extern char                     KERNEL_IOMMAP[];
extern char                     KERNEL_PHMMAP[];

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




/* maps a region of size bytes,                 *
 * returns KPAGE_ERR on error,                  *
 * or a valid ptr to address space on success.  *
 * phys is rounded down to nearest page size,   *
 * bytes is rounded up to multiple of pagesize. *
 * zero is a valid type for standard pages.     */
extern void *kmap(void *phys, void *virt, u64 bytes, enum addrspace_alloc_type type);


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
static inline u64 get_kernel_iomap(void)
{
        u64 x;
        __asm__ volatile ("movabs $KERNEL_IOMMAP, %0" : "=r"(x));
        return x;
}


/* linear mappings only. */

/* phys/virt conversion of linear ram map (at 0xFFFF800000000000) */
_const_ _always_inline_
static inline void *virt_to_phys_pm(void *v) { return (void *)((u64)v - get_kernel_phmap()); }
/* phys/virt conversion of linear ram map (at 0xFFFF800000000000) */
_const_ _always_inline_
static inline void *phys_to_virt_pm(void *p) { return (void *)((u64)p + get_kernel_phmap()); }
/* phys/virt conversion of linear kernel code map (at 0xFFFFFFFF80000000) */
_const_ _always_inline_
static inline void *virt_to_phys_of(void *v) { return (void *)((u64)v - get_kernel_offs()); }
/* phys/virt conversion of linear kernel code map (at 0xFFFFFFFF80000000) */
_const_ _always_inline_
static inline void *phys_to_virt_of(void *p) { return (void *)((u64)p + get_kernel_offs()); }

/* probably don't need virt/phys conversion for io. */



extern void kalloc_init(void);

