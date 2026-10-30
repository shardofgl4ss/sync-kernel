// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "vmalloc.h"

#include "pagetbl.h"
#include "slab.h"
#include "types.h"
#include "vma.h"

#include "mm/buddy.h"

static constexpr usize SZ_2M = 2ULL * 1024ULL * 1024ULL;
static constexpr usize SZ_1G = 1ULL * 1024ULL * 1024ULL * 1024ULL;


#define IS_ALIGNED(val, align) (((val) & ((align) - 1)) == 0)


/* The VMA flags and pagetable entry flags differ a bit. *
 * PTE flags:                                            *
 *	PT_FLAG_PRESENT                 = 1 << 0,        *
 *	PT_FLAG_WRITABLE                = 1 << 1,        *
 *	PT_FLAG_USER                    = 1 << 2,        *
 *	PT_FLAG_WRITE_THROUGH           = 1 << 3,        *
 *	PT_FLAG_PAGECACHE_DISABLE       = 1 << 4,        *
 *	PT_FLAG_DIRTY                   = 1 << 5,        *
 *      PT_FLAG_PAGESIZE                = 1 << 7,        *
 * And VMA: -------------------------------------------- *
 *	VM_READ		 = 1 << 0,                       *
 *	VM_WRITE	 = 1 << 1,                       *
 *	VM_EXEC		 = 1 << 2,                       *
 *	VM_USER		 = 1 << 3,                       *
 *	VM_ANON		 = 1 << 4,                       *
 *	VM_IO		 = 1 << 5,                       *
 *	VM_NOCACHE	 = 1 << 6,                       *
 *	VM_WRITE_THROUGH = 1 << 7,                       *
 *	VM_SHARED	 = 1 << 8,                       *
 *	VM_GROWSDOWN	 = 1 << 9,                       */
_const_
static u64 vma_to_pte_flags(vma_flags_t flags)
{
        u64 f = PT_FLAG_PRESENT;

        if (flags & VM_WRITE) f |= PT_FLAG_WRITABLE;
        if (flags & VM_USER) f |= PT_FLAG_USER;
        if (flags & VM_IO) f |= PT_FLAG_PRESENT | PT_FLAG_WRITABLE;
        if (flags & VM_NOCACHE) f |= PT_FLAG_PAGECACHE_DISABLE;
        if (flags & VM_WRITE_THROUGH) f |= PT_FLAG_WRITE_THROUGH;
        if (!(flags & VM_EXEC)) f |= (1ULL << 63);

        return f;
}


typedef struct vmalloc_page_ctx {
        const virt_addr_t vaddr;
        const phys_addr_t paddr;

        const usize align_sz;
        const u64 pte_flags;

        const usize l4_ent_count;
        const usize l3_ent_count;
        const usize l2_ent_count;
        const usize l1_ent_count;
} page_ctx_t;



static void pt_map_huge(PageTable *restrict l4, const page_ctx_t *restrict ctx)
{
        virt_addr_t vbase = ctx->vaddr;
        phys_addr_t pbase = ctx->paddr;

        usize l3_ent_rem = ctx->l3_ent_count;

        for (usize i = 0; i < ctx->l4_ent_count; i++) {
                const u16 l4_ent_idx = page_l4_idx((void *)vbase);
                PageTable *l3 = return_pt_or_alloc(&l4->entries[l4_ent_idx]);

                const usize l3_ent_count = l3_ent_rem > 512 ? 512 : l3_ent_rem;

                for (usize j = 0; j < l3_ent_count; j++) {
                        const usize l3_ent_idx = page_l3_idx((void *)vbase);
                        l3->entries[l3_ent_idx] = (pbase | ctx->pte_flags | PT_FLAG_PAGESIZE);

                        vbase += PT_L3_ENTRYSZ;
                        pbase += PT_L3_ENTRYSZ;
                }

                l3_ent_rem -= l3_ent_count;
        }
}
static void pt_map_large(PageTable *restrict l4, const page_ctx_t *restrict ctx)
{
        virt_addr_t vbase = ctx->vaddr;
        phys_addr_t pbase = ctx->paddr;

        usize l3_ent_rem = ctx->l3_ent_count;
        usize l2_ent_rem = ctx->l2_ent_count;

        for (usize i = 0; i < ctx->l4_ent_count; i++) {
                const u16 l4_ent_idx = page_l4_idx((void *)vbase);
                PageTable *l3 = return_pt_or_alloc(&l4->entries[l4_ent_idx]);

                const usize l3_ent_count = l3_ent_rem > 512 ? 512 : l3_ent_rem;

                for (usize j = 0; j < l3_ent_rem; j++) {
                        const u16 l3_ent_idx = page_l3_idx((void *)vbase);
                        PageTable *l2 = return_pt_or_alloc(&l3->entries[l3_ent_idx]);

                        const usize l2_ent_count = l2_ent_rem > 512 ? 512 : l2_ent_rem;

                        for (usize k = 0; k < l2_ent_count; k++) {
                                const u16 l2_ent_idx = page_l2_idx((void *)vbase);
                                l2->entries[l2_ent_idx] = pbase | ctx->pte_flags | PT_FLAG_PAGESIZE;

                                vbase += PT_L2_ENTRYSZ;
                                pbase += PT_L2_ENTRYSZ;
                        }

                        l2_ent_rem -= l2_ent_count;
                }

                l3_ent_rem -= l3_ent_count;
        }
}
static void pt_map_norm(PageTable *restrict l4, const page_ctx_t *restrict ctx)
{
        virt_addr_t vbase = ctx->vaddr;
        phys_addr_t pbase = ctx->paddr;

        usize l3_ent_rem = ctx->l3_ent_count;
        usize l2_ent_rem = ctx->l2_ent_count;
        usize l1_ent_rem = ctx->l1_ent_count;

        for (usize i = 0; i < ctx->l4_ent_count; i++) {
                const u16 l4_ent_idx = page_l4_idx((void *)vbase);
                PageTable *l3 = return_pt_or_alloc(&l4->entries[l4_ent_idx]);

                const usize l3_ent_count = l3_ent_rem > 512 ? 512 : l3_ent_rem;

                for (usize j = 0; j < l3_ent_count; j++) {
                        const u16 l3_ent_idx = page_l3_idx((void *)vbase);
                        PageTable *l2 = return_pt_or_alloc(&l3->entries[l3_ent_idx]);

                        const usize l2_ent_count = l2_ent_rem > 512 ? 512 : l2_ent_rem;

                        for (usize k = 0; k < l2_ent_count; k++) {
                                const u16 l2_ent_idx = page_l2_idx((void *)vbase);
                                PageTable *l1 = return_pt_or_alloc(&l2->entries[l2_ent_idx]);

                                const usize l1_ent_count = l1_ent_rem > 512 ? 512 : l1_ent_rem;

                                for (usize l = 0; l < l1_ent_count; l++) {
                                        const u16 l1_ent_idx = page_l1_idx((void *)vbase);
                                        l1->entries[l1_ent_idx] = pbase | ctx->pte_flags;

                                        vbase += PAGESIZE;
                                        pbase += PAGESIZE;
                                }

                                l1_ent_rem -= l1_ent_count;
                        }

                        l2_ent_rem -= l2_ent_count;
                }

                l3_ent_rem -= l3_ent_count;
        }
}




void *ioremap(phys_addr_t phy, usize size, vma_flags_t flags)
{
        usize offs = phy & (PAGESIZE - 1);
        usize aligned_size = PAGE_ALIGNUP(size + offs);

        phys_addr_t aligned_phys = PAGE_ALIGNDOWN(phy);

        kernel_vma_t *vma = vma_alloc_area(&vma_tree, aligned_size, flags, NULL);
        if (vma == VMA_INVALID_PTR) return KPAGE_ERR;

        const page_ctx_t ctx = {
                .paddr = aligned_phys,
                .vaddr = vma->vm_start,

                .align_sz = aligned_size,
                .pte_flags = vma_to_pte_flags(flags),

                .l4_ent_count = get_pagetbl_count_l3(aligned_size),
                .l3_ent_count = get_pagetbl_count_l2(aligned_size),
                .l2_ent_count = get_pagetbl_count_l1(aligned_size),
                .l1_ent_count = aligned_size >> 12

        };

        PageTable *restrict pml4 = kpt_get_l4();

        const bool hugepage_aligned = IS_ALIGNED(aligned_phys, SZ_1G) 
                && IS_ALIGNED(ctx.vaddr, SZ_1G) 
                && IS_ALIGNED(aligned_size, SZ_1G);

        const bool largepage_aligned = IS_ALIGNED(aligned_phys, SZ_2M) 
                && IS_ALIGNED(ctx.vaddr, SZ_2M) 
                && IS_ALIGNED(aligned_size, SZ_2M);

        if (hugepage_aligned) { 
                pt_map_huge(pml4, &ctx);
        } else if (largepage_aligned) { 
                pt_map_large(pml4, &ctx);
        } else { 
                pt_map_norm(pml4, &ctx);
        }

        return (void *)(ctx.vaddr + offs);
}




void *ioremap_prot(phys_addr_t phy, usize len, vma_flags_t flags)
{
        (void)phy;
        (void)len;
        (void)flags;
        return NULL;
        // usize offs = phy & (PAGESIZE - 1);
        // usize aligned_size = PAGE_ALIGNUP(size + offs);
        //
        // phys_addr_t aligned_phys = PAGE_ALIGNDOWN(phy);

}




void *iounmap(void *vaddr)
{
        (void)vaddr;
        return NULL;
}




/* singla page vmapping */
void *kmap(phys_addr_t phy)
{
        (void)phy;
        return NULL;
}




void kunmap(void *vaddr)
{
        (void)vaddr;
}




int vmalloc_init(void)
{
        return 0;
}

