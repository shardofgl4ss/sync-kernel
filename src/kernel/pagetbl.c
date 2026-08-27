// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "pagetbl.h"
#include "types.h"
#include "pframe.h"
#include "kstring.h"

#include "multiboot2.h"

typedef struct multiboot_tag_mmap mmap_t;

extern char             _begin[];
extern char             _after_ro[];
extern char             kstack_guard[];
extern char             kstack_top[];
extern char             _kheap[];

#define _Nullable
#define _Nonnull

#define KPT_NOT_PRESENT ((void *)-2)


extern struct kpage_core *PHYS_CORE;




static inline void tlb_invl_page(u64 vaddr)
{
        __asm__ volatile (
                "invlpg (%0)"
                :
                : "r"(vaddr)
                : "memory"
        );
}




static inline PageTable *kpt_get_l4(void)
{
	void *cr3;
        __asm__ volatile (
                "movq %%cr3, %0"
                : "=r"(cr3)
        );
        return PT_REBASE(cr3);
}
static PageTable *kpt_get_l3(void *vaddr, PageTable *_Nullable l4)
{
        if (l4) {
                return PT_REBASE(l4->entries[page_l4_idx(vaddr)]);
        }
        PageTable *pml4 = kpt_get_l4();
        return PT_REBASE(pml4->entries[page_l4_idx(vaddr)]);

}
static PageTable *kpt_get_l2(void *vaddr, PageTable *_Nullable l3)
{
        if (l3) {
                return PT_REBASE(l3->entries[page_l3_idx(vaddr)]);
        }
        PageTable *pdpt = kpt_get_l3(vaddr, NULL);
        u64 e = pdpt->entries[page_l3_idx(vaddr)];

        if (!(e & PT_FLAG_PRESENT))
                return KPT_NOT_PRESENT;
        if (e & PT_FLAG_PAGESIZE)
                return NULL;

        return PT_REBASE(e);
}
static PageTable *kpt_get_l1(void *vaddr, PageTable *_Nullable l2)
{
        if (l2) {
                return PT_REBASE(l2->entries[page_l2_idx(vaddr)]);
        }
        PageTable *pd = kpt_get_l2(vaddr, NULL);
        u64 e = pd->entries[page_l2_idx(vaddr)];

        if (!(e & PT_FLAG_PRESENT))
                return KPT_NOT_PRESENT;
        if (e & PT_FLAG_PAGESIZE)
                return NULL;

        return PT_REBASE(e);
}




// pagetables have given me an aneurysm
void *kpt_get_phys(void *vaddr)
{
        vaddr = (void *)PAGE_ALIGNDOWN((u64)vaddr);

        constexpr u64 bmask_51_21 = BITRANGE_MASK(51, 21);
        constexpr u64 bmask_51_30 = BITRANGE_MASK(51, 30);

        constexpr u64 bmask_29_00 = BITRANGE_MASK(29, 0);
        constexpr u64 bmask_20_00 = BITRANGE_MASK(20, 0);
        constexpr u64 bmask_11_00 = BITRANGE_MASK(11, 0);

        u64 entry = 0;

        const PageTable *pml4 = kpt_get_l4();
        entry = pml4->entries[page_l4_idx(vaddr)];
        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        const PageTable *pdpt = PT_REBASE(entry);
        entry = pdpt->entries[page_l3_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) 
                return KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                u64 paddr = (u64)page_phys_rebase((void *)entry, 30, bmask_51_30);
                paddr |= (u64)vaddr & bmask_29_00;
                return (void *)paddr;
        }

        const PageTable *pd = PT_REBASE(entry);
        entry = pd->entries[page_l2_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                u64 paddr = (u64)page_phys_rebase((void *)entry, 21, bmask_51_21);
                paddr |= (u64)vaddr & bmask_20_00;
                return (void *)paddr;
        }

        const PageTable *pt = PT_REBASE(entry);
        entry = pt->entries[page_l1_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        u64 paddr = (u64)PT_REBASE(entry);
	paddr |= (u64)page_phys_rebase(vaddr, 0, bmask_11_00);
	return (void *)paddr;
}




void virt_map_page(void *vaddr)
{
        (void)vaddr;
}




/* Unmaps the given 4kb memory region, rounded down to the nearest 4KB */
void virt_unmap_page(void *vaddr)
{
        if (unlikely(!vaddr)) return;
        usize valign = PAGE_ALIGNDOWN((usize)vaddr);
        void *v = (void *)valign;

        PageTable *l1 = kpt_get_l1(v, NULL);

        volatile u64 *restrict pte = &l1->entries[page_l1_idx(v)];

        *pte &= ~PT_FLAG_PRESENT;
        tlb_invl_page(valign);
}




void *kmap(void *phys, void *virt, u64 bytes, enum addrspace_alloc_type type)
{
        (void)phys;
        (void)virt;
        (void)bytes;
        (void)type;
        return NULL;
}




void kstack_guard_init(void)
{
        virt_unmap_page((void *)kstack_guard);
        virt_unmap_page((void *)kstack_top);
}




void kperm_init(void)
{
        const u64 dist = ((uintptr_t)_after_ro - (uintptr_t)_begin) / sizeof(PageTable);
        PageTable *arr = (void *)_begin;

        PageTable *l2 = kpt_get_l2(arr, NULL);

        void *v;
        for (u64 i = 0; i < dist; i++) {
                v = &arr[i];
                PageTable *l1 = kpt_get_l1(v, l2);
                l1->entries[page_l1_idx(v)] &= ~PT_FLAG_WRITABLE;
        }
}




static void unmap_preinit_mem(void)
{
        void *idmap = (void *)0x0;

        PageTable *pt_l4 = kpt_get_l4();
        PageTable *pt_l3 = kpt_get_l3(idmap, pt_l4);

        pt_l3->entries[0] &= ~PT_FLAG_PRESENT;
        pt_l4->entries[0] &= ~PT_FLAG_PRESENT;
        /* TODO unmap this dangling pagetable */
        tlb_invl_page((u64)idmap);
}




void kalloc_init()
{
        kstack_guard_init();
        kperm_init();
        // kphys_alloc_init();
        // unmap_preinit_mem();
        // kmem_pf_init();
        // physmap_init(map);
}


