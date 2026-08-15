#include "pagetbl.h"
#include "types.h"
#include "kstring.h"



extern char _begin[];
extern char _after_ro[];
extern char kstack_guard[];
extern char kstack_top[];



#define _KPAGE_         __attribute__((section(".kern_pt")))


_KPAGE_ PageTable KPAGETBL_L4;
_KPAGE_ PageTable KPAGETBL_L3;
_KPAGE_ PageTable KPAGETBL_L2;
_KPAGE_ PageTable KPAGETBL_L1[PAGE_PT_COUNT];


static inline void tlb_invl_page(u64 vaddr)
{
        __asm__ volatile (
                "invlpg (%0)"
                :
                : "r"(vaddr)
                : "memory"
        );
}

static inline void *pml4_addr_get(void)
{
        void *cr3;
        __asm__ volatile (
                "movq %%cr3, %0"
                : "=r"(cr3)
        );
        return cr3;
}



__attribute__((const)) //
static inline u64 page_l4_idx(void *vaddr) { return (((u64)vaddr >> 39) & 0x1FF); }
__attribute__((const)) //
static inline u64 page_l3_idx(void *vaddr) { return (((u64)vaddr >> 30) & 0x1FF); }
__attribute__((const)) //
static inline u64 page_l2_idx(void *vaddr) { return (((u64)vaddr >> 21) & 0x1FF); }
__attribute__((const)) //
static inline u64 page_l1_idx(void *vaddr) { return (((u64)vaddr >> 12) & 0x1FF); }
__attribute__((const)) //
static inline u64 page_offs_idx(void *vaddr) { return ((u64)vaddr & 0xFFF); }
__attribute__((const)) //
static inline void *page_phys_entry_rebase(void *paddr, u64 shift, u64 mask)
{
        return (void *)((((u64)paddr >> shift) & mask) << shift);
}

#define BITRANGE_MASK(a, b)     ((1ULL << ((a) - (b) + 1ULL)) - 1ULL)

// pagetables have given me an aneurysm
void *kpt_get_phys(void *vaddr)
{
        constexpr u64 bmask_page_entry = BITRANGE_MASK(51, 12);
        constexpr u64 page_entry_shift = 12;
        constexpr u64 bmask_51_21 = BITRANGE_MASK(51, 21);
        constexpr u64 bmask_51_30 = BITRANGE_MASK(51, 30);

        constexpr u64 bmask_29_00 = BITRANGE_MASK(29, 0);
        constexpr u64 bmask_20_00 = BITRANGE_MASK(20, 0);
        constexpr u64 bmask_11_00 = BITRANGE_MASK(11, 0);

        qword entry = 0;

        const PageTable *pml4 = page_phys_entry_rebase(pml4_addr_get(), page_entry_shift, bmask_page_entry);
        entry = pml4->entries[page_l4_idx(vaddr)];
        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        const PageTable *pdpt = page_phys_entry_rebase((void *)entry, page_entry_shift, bmask_page_entry);
        entry = pdpt->entries[page_l3_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) 
                return KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                u64 paddr = (u64)page_phys_entry_rebase((void *)entry, 30, bmask_51_30);
                paddr |= (u64)vaddr & bmask_29_00;
                return (void *)paddr;
        }

        const PageTable *pd = page_phys_entry_rebase((void *)entry, page_entry_shift, bmask_page_entry);
        entry = pd->entries[page_l2_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                u64 paddr = (u64)page_phys_entry_rebase((void *)entry, 21, bmask_51_21);
                paddr |= (u64)vaddr & bmask_20_00;
                return (void *)paddr;
        }

        const PageTable *pt = page_phys_entry_rebase((void *)entry, page_entry_shift, bmask_page_entry);
        entry = pt->entries[page_l1_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        u64 paddr = (u64)page_phys_entry_rebase((void *)entry, page_entry_shift, bmask_page_entry);
	paddr |= (u64)page_phys_entry_rebase(vaddr, 0, bmask_11_00);
	return (void *)paddr;
}


void kpt_uhalf_init(void)
{
        extern PageTable _PAGE_PML4;
        extern PageTable _PAGE_PDPT;
        extern PageTable _PAGE_PD;
        extern PageTable _PAGE_PT[PAGE_PT_COUNT];

        memcpy(&KPAGETBL_L4, &_PAGE_PML4, sizeof(PageTable));
        memcpy(&KPAGETBL_L3, &_PAGE_PDPT, sizeof(PageTable));
        memcpy(&KPAGETBL_L2, &_PAGE_PD, sizeof(PageTable));
        memcpy(KPAGETBL_L1, _PAGE_PT, sizeof(PageTable) * PAGE_PT_COUNT);

        // unmapping the 2mb identity map region.
        // vga memory will have to be remapped.
        KPAGETBL_L3.entries[0] &= ~(PT_FLAG_PRESENT | PT_FLAG_PAGESIZE);
}

void kperm_init(void)
{
        const u64 dist = ((uintptr_t)_after_ro - (uintptr_t)_begin) / sizeof(PageTable);
        PageTable *arr = (void *)_begin;

        void *v;
        for (u64 i = 0; i < dist; i++) {
                v = &arr[i];
                KPAGETBL_L1[page_l2_idx(v)].entries[page_l1_idx(v)] &= ~PT_FLAG_WRITABLE;
        }
}


/* Unmaps the given 4kb memory region, rounded down to the nearest 4KB */
void virt_unmap_page(void *vaddr)
{
        u64 palign = (u64)vaddr & ~0xFFF;

        void *p = (void *)palign;
        volatile qword *restrict pte = &KPAGETBL_L1[page_l2_idx(p)].entries[page_l1_idx(p)];
        *pte &= ~PT_FLAG_PRESENT;

        tlb_invl_page(palign);
}



void kstack_guard_init(void)
{
        virt_unmap_page((void *)kstack_guard);
        virt_unmap_page((void *)kstack_top);
}



void kpage_init(void)
{
        kpt_uhalf_init();
        kstack_guard_init();
        kperm_init();
}


