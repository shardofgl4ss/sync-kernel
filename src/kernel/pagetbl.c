#include "pagetbl.h"
#include "types.h"
#include "kmem.h"
#include "kstring.h"

#include "multiboot2.h"

typedef struct multiboot_tag_mmap mmap_t;

extern char KERNEL_OFFSET[];
extern char _begin[];
extern char _after_ro[];
extern char kstack_guard[];
extern char kstack_top[];
extern char _kheap[];

#define _Nullable
#define _Nonnull

#define PHYSMAP         ((u64)KERNEL_OFFSET)


extern struct kpage_core *PHYS_CORE;


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
constexpr u64 SHIFT_PGENTRY = 12;
constexpr u64 BMASK_PGENTRY = BITRANGE_MASK(51, 12);

#define PT_REBASE(p)            (void *)page_phys_rebase( \
                (void *)(p), \
                SHIFT_PGENTRY, \
                BMASK_PGENTRY)

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
        return PT_REBASE(pdpt->entries[page_l3_idx(vaddr)]);
}
static PageTable *kpt_get_l1(void *vaddr, PageTable *_Nullable l2)
{
        if (l2) {
                return PT_REBASE(l2->entries[page_l2_idx(vaddr)]);
        }
        PageTable *pd = kpt_get_l2(vaddr, NULL);
        return PT_REBASE(pd->entries[page_l2_idx(vaddr)]);
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


void kpt_uhalf_init(void)
{
        // extern PageTable _PAGE_PML4;
        // extern PageTable _PAGE_PDPT;
        // extern PageTable _PAGE_PD;
        // extern PageTable _PAGE_PT[PAGE_PT_COUNT];
        //
        // memcpy(&KPAGETBL_L4, &_PAGE_PML4, sizeof(PageTable));
        // memcpy(&KPAGETBL_L3, &_PAGE_PDPT, sizeof(PageTable));
        // memcpy(&KPAGETBL_L2, &_PAGE_PD, sizeof(PageTable));
        // memcpy(KPAGETBL_L1, _PAGE_PT, sizeof(PageTable) * PAGE_PT_COUNT);
        //
        // // unmapping the 2mb identity map region.
        // // vga memory will have to be remapped.
        // KPAGETBL_L3.entries[0] &= ~(PT_FLAG_PRESENT | PT_FLAG_PAGESIZE);
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


void virt_map_page(void *vaddr)
{
        (void)vaddr;
}


/* Unmaps the given 4kb memory region, rounded down to the nearest 4KB */
void virt_unmap_page(void *vaddr)
{
        if (!vaddr) return;
        usize valign = PAGE_ALIGNDOWN((usize)vaddr);
        void *v = (void *)valign;

        PageTable *l1 = kpt_get_l1(v, NULL);

        volatile u64 *restrict pte = &l1->entries[page_l1_idx(v)];

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
        void *first_pg = (void *)((u8 *)_kheap);
        u64 valign = PAGE_ALIGNDOWN((u64)first_pg);
        void *v = (void *)valign;

        PageTable *l1 = kpt_get_l1(v, NULL);
        volatile u64 *restrict pte = &l1->entries[page_l1_idx(v)];
        *pte = ((u64)virt_to_phys(first_pg)) | PT_FLAG_PRESENT | PT_FLAG_WRITABLE;

        PHYS_CORE = first_pg;
        memset(PHYS_CORE, 0, PAGESIZE);

        const u8 *end = (u8 *)map + map->size;

        extern char _kphys_start[];
        extern char _kphys_end[];

        const u8 *kend = (u8 *)_kphys_end + KERN_START_MEMB;
        const usize ksize = kend - (u8 *)_kphys_start;

        int bmi = 0;
        for (u8 *p = (u8 *)map + sizeof(*map); p < end; p += map->entry_size) {
                const struct multiboot_mmap_entry *e = (void *)p;
                if (e->type == MULTIBOOT_MEMORY_AVAILABLE) {

                        // for the kernel region itself we should put it after the kernel.
                        if (e->addr <= (u64)_kphys_start 
                                        && (u64)kend <= e->addr + e->len)
                        {
                                PHYS_CORE->phypool.region_addr[bmi] = (u64)((u8 *)e->addr + (u64)(kend - e->addr));
                                PHYS_CORE->phypool.region_size[bmi] = e->len - ksize;
                        } else {
                                PHYS_CORE->phypool.region_addr[bmi] = e->addr;
                                PHYS_CORE->phypool.region_size[bmi] = e->len;
                        }

                        PHYS_CORE->phypool.regions++;
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


