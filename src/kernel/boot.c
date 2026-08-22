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
        u8 *p = (u8 *)mbh + 8;
        u8 *end = mbh + *(u32 *)mbh;

        while (t < end) {
                struct multiboot_tag *tag = (void *)p;

                if (tag->type == type) {
                        return tag;
                } 

                if (tag->type == MULTIBOOT_TAG_TYPE_END) {
                        break;
                }

                p += (tag->size + 7) & ~7;
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
        PageTable *pagetbl;

        if (!(*pte & PT_FLAG_PRESENT)) {
                pagetbl = alloc_frame();
                *pte = (u64)pagetbl | PT_FLAGS_RW;
        } else {
                pagetbl = PT_REBASE(*pte);
        }

        return pagetbl;
}




static constexpr usize PT_ENTRIES = 512;


static constexpr usize PT_L1_ENTRYSZ = 4096;
static constexpr usize PT_L2_ENTRYSZ = PT_L1_ENTRYSZ * PT_ENTRIES;
static constexpr usize PT_L3_ENTRYSZ = PT_L2_ENTRYSZ * PT_ENTRIES;
static constexpr usize PT_L4_ENTRYSZ = PT_L3_ENTRYSZ * PT_ENTRIES;

static constexpr usize PT_L2_ENTRYMASK = PT_L2_ENTRYSZ - 1;
static constexpr usize PT_L3_ENTRYMASK = PT_L3_ENTRYSZ - 1;
static constexpr usize PT_L4_ENTRYMASK = PT_L4_ENTRYSZ - 1;

_PREINIT_ __attribute__((nonnull(1, 2)))
static void pt_map_fill_l1(PageTable *const restrict pt_l1, void **phys)
{
        for (usize i = 0; i < PT_ENTRIES; i++) {
                pt_l1->entries[i] = ((u64)(*phys) | PT_FLAGS_RW);
                *phys = ((u8 *)*phys) + PT_L1_ENTRYSZ;
        }
}




_PREINIT_ __attribute__((nonnull(1)))
static void pt_init_dram_map(PageTable *const pt_l4)
{
        extern char KERNEL_PHMMAP[];

        void *dram_map_offs;

        /* fuck you, compiler. */
        __asm__ volatile ("movabsq $KERNEL_PHMMAP, %0" : "=r"((u64)dram_map_offs) : : "memory");

	const usize ramsz = preinit_pfa.usable_ram;

	const usize pt_l3_count = ((ramsz + PT_L4_ENTRYMASK) & ~PT_L4_ENTRYMASK) / PT_L4_ENTRYSZ;
        const usize pt_l2_count = ((ramsz + PT_L3_ENTRYMASK) & ~PT_L3_ENTRYMASK) / PT_L3_ENTRYSZ;
        const usize pt_l1_count = ((ramsz + PT_L2_ENTRYMASK) & ~PT_L2_ENTRYMASK) / PT_L2_ENTRYSZ;

        void *physbase = (void *)0x0;

        /* REMINDER: make these loops actually stop at 512 per pagetable */
        for (usize i = 0; i < pt_l3_count; i++) {
                const u16 l4_entry_idx = page_l4_idx(dram_map_offs);
                PageTable *pt_l3 = chk_pt_alloc(&pt_l4->entries[l4_entry_idx]);

                for (usize j = 0; j < pt_l2_count; j++) {
                        const u16 l3_entry_idx = page_l3_idx(dram_map_offs);
                        PageTable *pt_l2 = chk_pt_alloc(&pt_l3->entries[l3_entry_idx]);

                        for (usize k = 0; k < pt_l1_count; k++) {
                                const u16 l2_entry_idx = page_l2_idx(dram_map_offs);
                                PageTable *pt_l1 = chk_pt_alloc(&pt_l2->entries[l2_entry_idx]);

                                pt_map_fill_l1(pt_l1, &physbase);
                                dram_map_offs = ((u8 *)dram_map_offs) + PT_L2_ENTRYSZ;
                        }
                }
        }
}
_PREINIT_ __attribute__((nonnull(1)))
static void pt_init_kcode(PageTable *const pt_l4)
{
        /* this is basically just another direct map, --------- *
         * but it only goes upto the size of the kernel image.  */
        const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;
        const u64 pt_l1_count = ((kernsz + PT_L2_ENTRYMASK) & ~PT_L2_ENTRYMASK) / PT_L2_ENTRYSZ;

        if (pt_l1_count > PT_ENTRIES) die();

        void *koffs = (void *)KERNEL_OFFSET;

        u16 l4_entry_idx = page_l4_idx(koffs);
        u16 l3_entry_idx = page_l3_idx(koffs);
        u16 l2_entry_idx = page_l2_idx(koffs);
        
        PageTable *pt_l3 = chk_pt_alloc(&pt_l4->entries[l4_entry_idx]);
        PageTable *pt_l2 = chk_pt_alloc(&pt_l3->entries[l3_entry_idx]);

        u64 physbase = 0x0;

        /* This is hard coded to have only one PD for kernel code. It is about  *
         * 1gb. It should be enough. If not, it needs refactored. ------------- */
        for (usize i = 0; i < pt_l1_count; i++) {
                u64 *const restrict pd_e = &pt_l2->entries[l2_entry_idx + i];
                PageTable *pt_l1 = chk_pt_alloc(pd_e);

                *pd_e = (u64)pt_l1 | PT_FLAGS_RW;
                for (usize j = 0; j < PT_ENTRIES; j++) {
                        pt_l1->entries[j] = physbase | PT_FLAGS_RW;
                        physbase += sizeof(PageTable);
                }
        }

        
}
_PREINIT_ __attribute__((nonnull(1)))
static void pt_init_temp_identmap(PageTable *pt_l4)
{
        u64 *const restrict pt_l4_entry = &pt_l4->entries[0];
        PageTable *pt_l3 = chk_pt_alloc(pt_l4_entry);

        if (unlikely(pt_l3 == KPAGE_ERR)) die();

        pt_l4->entries[0] = (u64)pt_l3 | PT_FLAGS_RW;
        pt_l3->entries[0] = (0x0ULL | PT_FLAGS_RW | PT_FLAG_PAGESIZE);
}




/* pagetables are the bane of my existence. */
_PREINIT_
static void init_higher_half(void)
{
        PageTable *pt_l4 = alloc_frame();

        pt_init_temp_identmap(pt_l4);
        set_pagetable(pt_l4);
        pt_init_kcode(pt_l4);
        pt_init_dram_map(pt_l4);
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

