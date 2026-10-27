// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "pagetbl.h"

#include "types.h"
#include "kstring.h"
#include "boot.h"

#include "mm/buddy.h"
#include "mm/slab.h"

#include "multiboot2.h"

typedef struct multiboot_tag_mmap mmap_t;

extern char _begin[];
extern char _after_ro[];
extern char kstack_guard[];
extern char kstack_top[];
extern char _kheap[];
extern char _kphys_start[];
extern char _kphys_end[];

#define _Nullable
#define _Nonnull

#define KPT_NOT_PRESENT ((void *)-1)

// static struct kslab_cache pt_cache = {};
// static const char *PT_CACHE_IDENTIFIER = "page_tbl_struct";




PageTable *kpt_get_l4(void)
{
	phys_addr_t cr3;
        __asm__ volatile (
                "movq %%cr3, %0"
                : "=r"(cr3)
        );
        return phys_to_virt_pm(PT_REBASE(cr3));
}
PageTable *kpt_get_l3(void *vaddr, PageTable *_Nullable l4)
{
        if (l4) {
                return phys_to_virt_pm(PT_REBASE(l4->entries[page_l4_idx(vaddr)]));
        }
        PageTable *pml4 = kpt_get_l4();
        return phys_to_virt_pm(PT_REBASE(pml4->entries[page_l4_idx(vaddr)]));

}
PageTable *kpt_get_l2(void *vaddr, PageTable *_Nullable l3)
{
        if (l3) {
                return phys_to_virt_pm(PT_REBASE(l3->entries[page_l3_idx(vaddr)]));
        }
        PageTable *pdpt = kpt_get_l3(vaddr, NULL);
        u64 e = pdpt->entries[page_l3_idx(vaddr)];

        if (!(e & PT_FLAG_PRESENT))
                return KPT_NOT_PRESENT;
        if (e & PT_FLAG_PAGESIZE)
                return NULL;

        return phys_to_virt_pm(PT_REBASE(e));
}
PageTable *kpt_get_l1(void *vaddr, PageTable *_Nullable l2)
{
        if (l2) {
                return phys_to_virt_pm(PT_REBASE(l2->entries[page_l2_idx(vaddr)]));
        }
        PageTable *pd = kpt_get_l2(vaddr, NULL);
        u64 e = pd->entries[page_l2_idx(vaddr)];

        if (!(e & PT_FLAG_PRESENT))
                return KPT_NOT_PRESENT;
        if (e & PT_FLAG_PAGESIZE)
                return NULL;

        return phys_to_virt_pm(PT_REBASE(e));
}




/* Returns a physmap ptr to a valid page table if it exists. If not, this allocates one and returns it. */
PageTable *return_pt_or_alloc(u64 *const restrict pte)
{
        if (!(*pte & PT_FLAG_PRESENT)) {
                PageTable *p = kbud_alloc(BUDDY_ORDER_4K);
                if (p == KPAGE_ERR) return KPAGE_ERR;

                memset((void *)p, 0, sizeof(PageTable));
                *pte = (u64)virt_to_phys_pm(p) | PT_FLAG_PRESENT | PT_FLAG_WRITABLE;
                return p;
        } else {
                phys_addr_t phy = PT_REBASE(*pte);
                return phys_to_virt_pm(phy);
        }

}




// pagetables have given me an aneurysm
phys_addr_t kpt_get_phys(void *vaddr)
{
        // vaddr = (void *)PAGE_ALIGNDOWN((u64)vaddr);

        constexpr u64 bmask_51_21 = BITRANGE_MASK(51, 21);
        constexpr u64 bmask_51_30 = BITRANGE_MASK(51, 30);
        constexpr u64 bmask_29_00 = BITRANGE_MASK(29, 00);
        constexpr u64 bmask_20_00 = BITRANGE_MASK(20, 00);
        constexpr u64 bmask_11_00 = BITRANGE_MASK(11, 00);

        u64 entry = 0;

        const PageTable *pml4 = kpt_get_l4();
        entry = pml4->entries[page_l4_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) return (phys_addr_t)KPAGE_ERR;

        const PageTable *pdpt = phys_to_virt_pm(PT_REBASE(entry));
        entry = pdpt->entries[page_l3_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) return (phys_addr_t)KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                phys_addr_t paddr = page_phys_rebase(entry, 30, bmask_51_30);
                paddr |= (phys_addr_t)((uintptr_t)vaddr & bmask_29_00);
                return paddr;
        }

        const PageTable *pd = phys_to_virt_pm(PT_REBASE(entry));
        entry = pd->entries[page_l2_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) return (phys_addr_t)KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                phys_addr_t paddr = page_phys_rebase(entry, 21, bmask_51_21);
                paddr |= (phys_addr_t)((uintptr_t)vaddr & bmask_20_00);
                return paddr;
        }

        const PageTable *pt = phys_to_virt_pm(PT_REBASE(entry));
        entry = pt->entries[page_l1_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) return (phys_addr_t)KPAGE_ERR;

        phys_addr_t paddr = PT_REBASE(entry);
	paddr |= (phys_addr_t)((uintptr_t)vaddr & bmask_11_00);
	return paddr;
}




/* Unmaps the given 4kb memory region, rounded down to the nearest 4KB */
void kpt_unmap_page(void *vaddr)
{
        if (unlikely(!vaddr)) return;
        usize valign = PAGE_ALIGNDOWN((uintptr_t)vaddr);
        void *v = (void *)valign;

        PageTable *l1 = kpt_get_l1(v, NULL);

        volatile u64 *restrict pte = &l1->entries[page_l1_idx(v)];

        *pte &= ~PT_FLAG_PRESENT;
        tlb_invl_page(valign);
}




void kstack_guard_init(void)
{
        kpt_unmap_page((void *)kstack_guard);
        kpt_unmap_page((void *)kstack_top);
}




static void remap_bootmem_kcode_pages(PageTable *l4, void *offs, const usize kernsz)
{
        phys_addr_t phy = 0x0ULL;

        const usize l1_count = ((kernsz + PT_L2_ENTRYMASK) & ~PT_L2_ENTRYMASK) / PT_L2_ENTRYSZ;
        const usize l2_count = ((kernsz + PT_L3_ENTRYMASK) & ~PT_L3_ENTRYMASK) / PT_L3_ENTRYSZ;
        
        // reminder: each l3 entry is 1gb, the whole l3 is 512
        // like the impl in boot.c, this is fixed at 1gb atm.
        PageTable *l3 = return_pt_or_alloc(&l4->entries[page_l4_idx(offs)]);

        PageTable *l1[l1_count] = {};
        PageTable *l2[l2_count] = {};

        for (usize i = 0; i < l2_count; i++) {
                l2[i] = return_pt_or_alloc(&l3->entries[page_l3_idx(offs + (i * PT_L3_ENTRYSZ))]);
                PageTable *pd = l2[i];

                for (usize j = 0; j < l1_count; j++) {
                        l1[j] = return_pt_or_alloc(&pd->entries[page_l2_idx(offs + (j * PT_L2_ENTRYSZ))]);
                        PageTable *pt = l1[j];

                        for (usize k = 0; k < PT_ENTRIES; k++) {
                                u64 flags = PT_FLAG_PRESENT;
                                void *addr = phys_to_virt_of(phy);

                                if (addr < (void *)_begin || addr >= (void *)_after_ro)
                                        flags |= PT_FLAG_WRITABLE;

                                pt->entries[k] = phy | flags;
                                phy += sizeof(PageTable);
                        }
                }
        }
}

static void remap_bootmem_dmap_pages(PageTable *l4, void *dmap, const usize ramsz)
{
        void *phys = (void *)0x0ULL;

	const usize pt_l3_count = ((ramsz + PT_L4_ENTRYMASK) & ~PT_L4_ENTRYMASK) / PT_L4_ENTRYSZ;
        const usize pt_l2_count = ((ramsz + PT_L3_ENTRYMASK) & ~PT_L3_ENTRYMASK) / PT_L3_ENTRYSZ;

        usize rem = pt_l2_count;

        for (usize i = 0; i < pt_l3_count; i++) {
                const u16 idx_l4 = page_l4_idx(dmap);
                PageTable *pdpt = return_pt_or_alloc(&l4->entries[idx_l4]);

                const usize count = rem > 512 ? 512 : rem;

                for (usize j = 0; j < count; j++) {
                        const u16 idx_l3 = page_l3_idx(dmap);
                        pdpt->entries[idx_l3] = ((u64)phys) | (PT_FLAG_PRESENT | PT_FLAG_WRITABLE | PT_FLAG_PAGESIZE);

                        phys = (char *)phys + PT_L3_ENTRYSZ;
                        dmap = (char *)dmap + PT_L3_ENTRYSZ;
                }

                rem -= count;
        }
}




/* All preinit data and code after this function is reused memory. */
static void reallocate_bootmem_pages(void)
{
        extern char __prebegin[];
        extern char __preend[];

        const u64 kernsz = (u64)_kphys_end;
        const usize ram_max = preinit_pfa.total_ram;
        
        const usize len = preinit_pfa.region[0]->max_frames << 12;
        void *vbase = phys_to_virt_pm((phys_addr_t)preinit_pfa.region[0]->base);

        void *dmap = (void *)get_kernel_phmap();
        void *offs = (void *)get_kernel_offs();

        PageTable *pml4 = kbud_alloc(BUDDY_ORDER_4K);
        // TODO: add a kernel warning here to make it clear that reallocation has failed, but it is not fatal.
        // it may be though, cause slabcache creation shouldn't fail if not OOM. memory too fragged? This early?
        if (pml4 == KPAGE_ERR) return;
        memset((void *)pml4, 0, sizeof(PageTable));
        
        remap_bootmem_kcode_pages(pml4, offs, kernsz);
        remap_bootmem_dmap_pages(pml4, dmap, ram_max);

        __asm__ volatile (
                "mov %0, %%cr3"
                :
                : "r"(virt_to_phys_pm(pml4))
                : "memory"
        );

        // struct preinit_allocator *early_alloc = phys_to_virt_pm((void *)&preinit_pfa);
        // physmem_pregion_t *region = phys_to_virt_pm(early_alloc->region[0]);
        // const usize len = region->max_frames << 12;

        kbud_free_range(vbase, len);

        const uintptr_t preinit_memory = (uintptr_t)__preend - (uintptr_t)__prebegin;
        void *base = phys_to_virt_of((uintptr_t)__prebegin);

        memset(base, -1, preinit_memory);
        kbud_free_range(base, preinit_memory);
}




int kern_uhalf_init()
{
        parse_mb2_entries();
        reallocate_bootmem_pages();
        kstack_guard_init();

        return 0;
}

