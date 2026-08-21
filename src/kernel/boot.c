#include "boot.h"

#include "macros.h"
#include "types.h"
#include "pagetbl.h"
#include "pframe.h"

#include <multiboot2.h>


#define _PREINIT_               __attribute__((section(".preinit")))
#define _PREINIT_DATA_          __attribute__((section(".preinit.data")))
#define _PREINIT_BSS_           __attribute__((section(".preinit.bss")))


typedef struct multiboot_mmap_entry mapentry_t;
extern _Noreturn void kernel_main(void);

_PREINIT_BSS_
struct kpage_core preinit_pfa = {};
_PREINIT_BSS_
struct multiboot_tag_mmap *multiboot2_tmmap = NULL;

extern char _kphys_start[];
extern char _kphys_end[];

extern char KERNEL_OFFSET[];
extern char kstack_top[];

extern char _kheap[];

static constexpr uint8_t PT_FLAGS_RW = PT_FLAG_PRESENT | PT_FLAG_WRITABLE;


_PREINIT_
static void pi_memset(void *dst, u8 c, usize n)
{
        u8 *dest = dst;
        for (u32 i = 0; i < n; i++) {
                dest[i] = c;
        }
}


_PREINIT_
_Noreturn static inline void die(void)
{
        __asm__ volatile ("cli"); 
        for (;;) __asm__ volatile ("hlt");
}


_PREINIT_
static inline void debug(void)
{
        __asm__ volatile (
                "outb %%al, $0xe9"
                :
                : "a"((u8)'A')
        );
}


_PREINIT_
static inline void set_pagetable(PageTable *pml4)
{
        __asm__ volatile (
                "mov %0, %%cr3"
                :
                : "r"(pml4)
                : "memory"
        );
}


_PREINIT_ __attribute__((noreturn, naked))
_Noreturn static void trampoline()
{
        __asm__ volatile (
                "movq $kstack_top, %rsp\n\t"
                "andq $-0x10, %rsp\n\t"
                "pushq $0\n\t"

                "jmp kernel_main"
        );
}




_PREINIT_
static struct multiboot_tag *find_mbt(void *mbh, u32 type)
{
        struct multiboot_tag *t = (void *)((u8 *)mbh + 8);

        while ((void *)t < (void *)((u8 *)mbh + (u32)((u64)mbh))) {
                if (t->type == type) return t;
                t = (void *)((u8 *)t + t->size);

                if ((uintptr_t)t % 8 > 0) 
                        t = (void *)((u8 *)t + 8 - ((uintptr_t)t % 8));
        }
        return NULL;
}




_PREINIT_
static void pf_init_region(const usize region, const mapentry_t *restrict ent)
{
        physmem_region_t *r = &preinit_pfa.region[region];

        const u64 entry_start = ent->addr;
        const u64 entry_end   = ent->addr + ent->len;
        const u64 ksize = (u64)_kphys_end - (u64)_kphys_start;


        if (unlikely(entry_start <= (u64)_kphys_start
                    && entry_end >= (u64)_kphys_end))
        {
                r->base = (void *)(ent->addr + ksize);
                r->max_frames = (ent->len - ksize) / PAGESIZE;
        } else {
                r->base = (void *)ent->addr;
                r->max_frames = ent->len / PAGESIZE;
        }


        r->cur_frames = r->max_frames;
        r->top = NULL;
        preinit_pfa.regions++;
}




_PREINIT_
static void alloc_preinit(struct multiboot_tag_mmap *map)
{
        if (unlikely(map == NULL)) die();

        static constexpr usize min_ram_sz = 1024 * 128;

        const u8 *end = (u8 *)map + map->size;
        usize *ramsz  = &preinit_pfa.usable_ram;
        bool done     = false;

        for (u8 *p = (u8 *)map + sizeof(*map); p < end; p += map->entry_size) {
                const mapentry_t *restrict ent = (void *)p;

                if (ent->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
                        *ramsz += ent->len;
                        continue;
                }

                if (ent->type != MULTIBOOT_MEMORY_AVAILABLE) {
                        continue;
                }

                *ramsz += ent->len;
                /* we should keep looping through entries to get the maximum ram only once.  */
                if (done) continue;

                /* The kernel needs at minimum 128kb of extra space. starting with less is shit. */
                if (ent->len < min_ram_sz) {
                        continue;
                }

                pf_init_region(0, ent);
                done = true;
        }

        if (preinit_pfa.regions == 0) die();
}




_PREINIT_
static void alloc_init(struct multiboot_tag_mmap *map)
{
        alloc_preinit(map);

        if (unlikely(preinit_pfa.regions == 0)) die();

        physmem_region_t *r = &preinit_pfa.region[preinit_pfa.regions - 1];

        page_frame_t *last_frame = (page_frame_t *)r->base + (r->max_frames - 1);
        page_frame_t *first_frame = (page_frame_t *)r->base;
        
        first_frame->next = KPAGE_ERR;

        for (page_frame_t *pf = first_frame + 1; pf <= last_frame; pf++) {
                pf->next = pf - 1;
        }

        r->top = last_frame;
}




_PREINIT_ __attribute__((malloc))
static void *alloc_frame(void)
{
        physmem_region_t *r = &preinit_pfa.region[0];
        page_frame_t *p = r->top;

        /* 0x0 is a valid phys addr we can usually use, so we need to use something else. */
        if (unlikely(p == KPAGE_ERR)) {
                return KPAGE_ERR;
        }

        r->top = p->next;
        pi_memset(p, 0, PAGESIZE);
        r->cur_frames--;

        return p;
}




/* If a pagetable is present in the PTE, returns it, if not, allocates, sets and returns it. */
_PREINIT_ __attribute__((nonnull(1)))
static PageTable *chk_pt_alloc(u64 *const restrict pte)
{
        PageTable *pt;

        if (!(*pte & PT_FLAG_PRESENT)) {
                pt = alloc_frame();
                *pte = (u64)pt | PT_FLAGS_RW;
        } else {
                pt = PT_REBASE(*pte);
        }

        return pt;
}




_PREINIT_ __attribute__((nonnull(1)))
static void pt_init_dram_map(PageTable *const pml4)
{
        (void)pml4;
}
_PREINIT_ __attribute__((nonnull(1)))
static void pt_init_kcode(PageTable *const pml4)
{
        /* this is basically just another direct map, --------- *
         * but it only goes upto the size of the kernel image.  */
        constexpr usize PT_TOTAL_MEM = 1024 * 1024 * 2;
        constexpr usize PT_MEM_MASK = PT_TOTAL_MEM - 1;
        static constexpr usize PT_ENTRIES = 512;

        const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;
        const u64 kern_pt_count = ((kernsz + PT_MEM_MASK) & ~PT_MEM_MASK) / PT_TOTAL_MEM;

        if (kern_pt_count > PT_ENTRIES) die();

        void *koffs = (void *)KERNEL_OFFSET;

        u16 pml4_i = page_l4_idx(koffs);
        u16 pdpt_i = page_l3_idx(koffs);
        u16 pd_i = page_l2_idx(koffs);
        
        PageTable *pdpt = chk_pt_alloc(&pml4->entries[pml4_i]);
        PageTable *pd = chk_pt_alloc(&pdpt->entries[pdpt_i]);

        u64 base = 0x0;

        /* This is hard coded to have only one PD for kernel code. It is about  *
         * 1gb. It should be enough. If not, it needs refactored. ------------- */
        for (usize i = 0; i < kern_pt_count; i++) {
                u64 *const restrict pd_e = &pd->entries[pd_i + i];
                PageTable *pt = chk_pt_alloc(pd_e);

                *pd_e = (u64)pt | PT_FLAGS_RW;
                for (usize j = 0; j < PT_ENTRIES; j++) {
                        pt->entries[j] = base | PT_FLAGS_RW;
                        base += sizeof(PageTable);
                }
        }

        
}
_PREINIT_ __attribute__((nonnull(1)))
static void pt_init_temp_identmap(PageTable *pml4)
{
        u64 *const restrict pte = &pml4->entries[0];
        PageTable *pdpt = chk_pt_alloc(pte);

        if (unlikely(pdpt == KPAGE_ERR)) die();

        pml4->entries[0] = (u64)pdpt | PT_FLAGS_RW;
        pdpt->entries[0] = (0x0ULL | PT_FLAGS_RW | PT_FLAG_PAGESIZE);
}




/* pagetables are the bane of my existence. */
_PREINIT_
static void init_higher_half(void)
{
        PageTable *pml4 = alloc_frame();

        pt_init_temp_identmap(pml4);
        set_pagetable(pml4);
        pt_init_kcode(pml4);
        pt_init_dram_map(pml4);
}


_PREINIT_
_Noreturn void start_kernel(void *mbh)
{
        struct multiboot_tag_mmap *map = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_MMAP);
        multiboot2_tmmap = map;

        alloc_init(map);
        init_higher_half();
        trampoline();
}

