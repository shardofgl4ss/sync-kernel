#include "pagetbl.h"
#include "types.h"
#include "kstring.h"

#include "multiboot2.h"

typedef multiboot_tag_mmap mmap_t;

extern char KERNEL_OFFSET[];
extern char _begin[];
extern char _after_ro[];
extern char kstack_guard[];
extern char kstack_top[];
extern char _kheap[];


#define PHYSMAP         ((u64)KERNEL_OFFSET)

// #define _KPAGE_         __attribute__((section(".kern_pt")))


// the kernel just dies if there are more then 256 memory regions.
// it shouldn't ever happen anyway, even in huge memory systems.
#define KPG_MAX_REGIONS         4
#define REGION_TERMINATOR       ((void *)-1)

struct phys_mem_rpool {
        u64 region_size[sizeof(u64) * KPG_MAX_REGIONS];
        u64 region_addr[sizeof(u64) * KPG_MAX_REGIONS];
        // bitmap of each region for use in above arrays.
        u64 region_bmap[KPG_MAX_REGIONS];
        // actual region max
        int regions;
} __attribute__((aligned(PAGESIZE)));
_Static_assert(sizeof(struct phys_mem_rpool) == PAGESIZE,
	       "phys_memcore not equal to size of page!\n");


struct kpage_core {
        struct phys_mem_rpool *rpool;
};





// _KPAGE_ PageTable KPAGETBL_L4;
// _KPAGE_ PageTable KPAGETBL_L3;
// _KPAGE_ PageTable KPAGETBL_L2;
// _KPAGE_ PageTable KPAGETBL_L1[PAGE_PT_COUNT];


/* linear mapping only. */
__attribute__((const)) //
static inline void *virt_to_phys(void *v) { return (void *)((u64)v - PHYSMAP); }
/* linear mapping only. */
__attribute__((const)) //
static inline void *phys_to_virt(void *p) { return (void *)((u64)p + PHYSMAP); }




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
static inline void *page_phys_rebase(void *paddr, u64 shift, u64 mask)
{
        return (void *)((((u64)paddr >> shift) & mask) << shift);
}




#define BITRANGE_MASK(a, b)     ((1ULL << ((a) - (b) + 1ULL)) - 1ULL)

// pagetables have given me an aneurysm
void *kpt_get_phys(void *vaddr)
{
        vaddr = (void *)PAGE_ALIGNDOWN((u64)vaddr);

        constexpr u64 page_entry_shift = 12;
        constexpr u64 bmask_page_entry = BITRANGE_MASK(51, 12);
        constexpr u64 bmask_51_21 = BITRANGE_MASK(51, 21);
        constexpr u64 bmask_51_30 = BITRANGE_MASK(51, 30);

        constexpr u64 bmask_29_00 = BITRANGE_MASK(29, 0);
        constexpr u64 bmask_20_00 = BITRANGE_MASK(20, 0);
        constexpr u64 bmask_11_00 = BITRANGE_MASK(11, 0);

        u64 entry = 0;

        const PageTable *pml4 = page_phys_rebase(pml4_addr_get(), page_entry_shift, bmask_page_entry);
        entry = pml4->entries[page_l4_idx(vaddr)];
        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        const PageTable *pdpt = page_phys_rebase((void *)entry, page_entry_shift, bmask_page_entry);
        entry = pdpt->entries[page_l3_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0) 
                return KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                u64 paddr = (u64)page_phys_rebase((void *)entry, 30, bmask_51_30);
                paddr |= (u64)vaddr & bmask_29_00;
                return (void *)paddr;
        }

        const PageTable *pd = page_phys_rebase((void *)entry, page_entry_shift, bmask_page_entry);
        entry = pd->entries[page_l2_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        if ((entry & PT_FLAG_PAGESIZE) != 0) {
                u64 paddr = (u64)page_phys_rebase((void *)entry, 21, bmask_51_21);
                paddr |= (u64)vaddr & bmask_20_00;
                return (void *)paddr;
        }

        const PageTable *pt = page_phys_rebase((void *)entry, page_entry_shift, bmask_page_entry);
        entry = pt->entries[page_l1_idx(vaddr)];

        if ((entry & PT_FLAG_PRESENT) == 0)
                return KPAGE_ERR;

        u64 paddr = (u64)page_phys_rebase((void *)entry, page_entry_shift, bmask_page_entry);
	paddr |= (u64)page_phys_rebase(vaddr, 0, bmask_11_00);
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


void virt_map_page(void *vaddr)
{

}


/* Unmaps the given 4kb memory region, rounded down to the nearest 4KB */
void virt_unmap_page(void *vaddr)
{
        usize valign = PAGE_ALIGNDOWN((usize)vaddr)

        void *v = (void *)valign;
        volatile u64 *restrict pte = &KPAGETBL_L1[page_l2_idx(v)].entries[page_l1_idx(v)];
        *pte &= ~PT_FLAG_PRESENT;

        tlb_invl_page(valign);
}


void kstack_guard_init(void)
{
        virt_unmap_page((void *)kstack_guard);
        virt_unmap_page((void *)kstack_top);
}

void physmap_init(mmap_t *map) 
{
        void *first_pg = (void *)_kheap;
        u64 valign = PAGE_ALIGNDOWN((u64)first_pg);
        void *v = (void *)valign;
        volatile u64 *restrict pte = &KPAGETBL_L1[page_l2_idx(v)].entries[page_l1_idx(v)];

        *pte = ((u64)virt_to_phys(first_pg)) | PT_FLAG_PRESENT | PT_FLAG_WRITABLE;

        rpool = first_pg;
        memset(core, 0, PAGESIZE);

        int entries = map->size / map->entry_size;

        int bmi = 0;
        for (int i = 0; i < entries; i++) {
                multiboot_mmap_entry *e = map->entries[i];
                if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {
                        rpool->region_addr[bmi] = e->addr;
                        rpool->region_size[bmi] = e->len;

                        rpool->regions++;
                        bmi++;
                }
        }
}





void kpage_init(void *maphdr)
{
        mmap_t *map = maphdr;

        kpt_uhalf_init();
        kstack_guard_init();
        kperm_init();
        physmap_init(map);

}


