// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "boot.h"

#include "macros.h"
#include "types.h"
#include "pagetbl.h"
#include "pframe.h"
#include "kstring.h"

#include <multiboot2.h>


typedef struct multiboot_mmap_entry mapentry_t;
extern _Noreturn void kernel_main(void);

_preinit_zeroed_
struct preinit_allocator preinit_pfa = {};
_preinit_zeroed_
struct multiboot_tag_mmap *multiboot2_tmmap = NULL;

extern char _kphys_start[];
extern char _kphys_end[];

extern char KERNEL_OFFSET[];
extern char kstack_top[];

extern char _kheap[];

static constexpr uint8_t PT_FLAGS_RW = PT_FLAG_PRESENT | PT_FLAG_WRITABLE;


_preinit_ _hot_
static void pi_memset(void *dst, u8 c, usize n)
{
        u8 *dest = dst;
        for (u32 i = 0; i < n; i++) {
                dest[i] = c;
        }
}


/* errstr is entirely just for any debugger to see. */
_preinit_ _noreturn_
static inline void die(char *errstr)
{
        (void)errstr;
        __asm__ volatile ("cli"); 
        for (;;) __asm__ volatile ("hlt");
}


_preinit_
static inline void debug(void)
{
        __asm__ volatile (
                "outb %%al, $0xe9"
                :
                : "a"((u8)'A')
        );
}


_preinit_
static inline void set_pagetable(PageTable *pml4)
{
        __asm__ volatile (
                "mov %0, %%cr3"
                :
                : "r"(pml4)
                : "memory"
        );
}


_preinit_ _noreturn_ _naked_
static void trampoline()
{
        __asm__ volatile (
                "movq $kstack_top, %rsp\n\t"
                "andq $-0x10, %rsp\n\t"
                "pushq $0\n\t"

                "jmp kernel_main"
        );
}




_preinit_
static struct multiboot_tag *find_mbt(void *mbh, u32 type)
{
        u8 *p = (u8 *)mbh + 8;
        u8 *end = mbh + *(u32 *)mbh;

        while (p < end) {
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




_preinit_
static void pf_init_region(const usize region, const mapentry_t *restrict ent)
{
        const u64 entry_start = ent->addr;
        const u64 entry_end   = ent->addr + ent->len;
        const u64 kernel_sz   = (u64)_kphys_end - (u64)_kphys_start;

        void *addr = (void *)ent->addr;
        usize len = ent->len;

        if (unlikely(entry_start <= (u64)_kphys_start
                    && entry_end >= (u64)_kphys_end)) {
                addr = (u8 *)addr + kernel_sz;
                len -= kernel_sz;
        }

        preinit_pfa.region[region] = addr;
        physmem_pregion_t *r = preinit_pfa.region[region];

        r->base = r;
        r->max_frames = (PAGE_ALIGNDOWN(len)) / 4096;

        /* we need to reserve the first frame for the slave_buddy_t */
        r->cur_frames = r->max_frames - 1;
        r->top = ((page_frame_t *)((u8 *)r->base + len)) - 1;

        preinit_pfa.region_count++;
}




_preinit_
static void sort_region_addrs(physmem_pregion_t **r, usize up_to)
{
        for (usize i = 1; i < up_to; i++) {
                physmem_pregion_t *k = r[i];
                usize j = i;

                while (j > 0 && r[j - 1]->base > k->base) {
                        r[j] = r[j - 1];
                        j--;
                }

                r[j] = k;
        }
}




_preinit_
static void alloc_init(struct multiboot_tag_mmap *map)
{
        if (unlikely(map == NULL)) die("map is NULL at alloc_init()");

        pi_memset(preinit_pfa.region, (u8)-1, sizeof(preinit_pfa.region));

        u8 *begin = (u8 *)map + sizeof(*map);
        const u8 *end = (u8 *)map + map->size;

        int regions = 0;
        for (u8 *p = begin; p < end; p += map->entry_size) {
                const mapentry_t *const ent = (void *)p;

                preinit_pfa.total_ram += ent->len;

                if (ent->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
                        preinit_pfa.usable_ram += ent->len;
                }

                if (ent->type != MULTIBOOT_MEMORY_AVAILABLE) {
                        continue;
                }

                preinit_pfa.usable_ram += ent->len;

                pf_init_region(regions, ent);
                
                // cant remember if this is at the limit or one off. oh well.
                if (++regions > PFRAME_MAX_REGIONS - 1) {
                        die("preinit memory regions exceeded capacity");
                }
        }
        
        sort_region_addrs(preinit_pfa.region, preinit_pfa.region_count);
}




_preinit_ _malloc_ _assume_aligned_(4096)
static void *alloc_frame(void)
{
        physmem_pregion_t **pr = &preinit_pfa.region[0];
        page_frame_t *frame = KPAGE_ERR;


        for (; *pr != KPAGE_ERR; pr++) {
                physmem_pregion_t *r = *pr;

                if (r->cur_frames == 0) continue;
                
                frame = r->top;

                r->top--;
                r->cur_frames--;

                pi_memset(frame, 0, PAGESIZE);
                return frame;
        }

        return KPAGE_ERR;
}




/* If a pagetable is present in the PTE, returns it, if not, allocates, sets and returns it. */
_preinit_ _nonnull_(1)
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




_preinit_ _nonnull_(1)
static void pt_init_dram_map(PageTable *const pt_l4)
{
        void *dram_map_offs = (void *)get_kernel_phmap();

	const usize ramsz = preinit_pfa.total_ram;

	const usize pt_l3_count = ((ramsz + PT_L4_ENTRYMASK) & ~PT_L4_ENTRYMASK) / PT_L4_ENTRYSZ;
        const usize pt_l2_count = ((ramsz + PT_L3_ENTRYMASK) & ~PT_L3_ENTRYMASK) / PT_L3_ENTRYSZ;

        void *physbase = (void *)0x0;
        usize rem = pt_l2_count;

        /* granularity isn't needed for the ram map, so we just use huge pages. */
        for (usize i = 0; i < pt_l3_count; i++) {
                const u16 l4_entry_idx = page_l4_idx(dram_map_offs);
                PageTable *pt_l3 = chk_pt_alloc(&pt_l4->entries[l4_entry_idx]);

                const usize cnt = rem > 512 ? 512 : rem;

                for (usize j = 0; j < cnt; j++) {
                        const u16 l3_entry_idx = page_l3_idx(dram_map_offs);
                        pt_l3->entries[l3_entry_idx] = ((u64)physbase | PT_FLAGS_RW | PT_FLAG_PAGESIZE);

                        physbase += PT_L3_ENTRYSZ;
                        dram_map_offs = (u8 *)dram_map_offs + PT_L3_ENTRYSZ;
                }

                rem -= cnt;
        }
}
_preinit_ _nonnull_(1)
static void pt_init_kcode(PageTable *const pt_l4)
{
        /* this is basically just another direct map, --------- *
         * but it only goes upto the size of the kernel image.  */
        const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;
        const u64 pt_l1_count = ((kernsz + PT_L2_ENTRYMASK) & ~PT_L2_ENTRYMASK) / PT_L2_ENTRYSZ;

        if (pt_l1_count > PT_ENTRIES) die("pt_init_kcode() needs some refactoring!");

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
_preinit_ _nonnull_(1)
static void pt_init_temp_identmap(PageTable *pt_l4)
{
        u64 *const restrict pt_l4_entry = &pt_l4->entries[0];
        PageTable *pt_l3 = chk_pt_alloc(pt_l4_entry);

        if (unlikely(pt_l3 == KPAGE_ERR)) die("pt_l3 is an invalid ptr!!");

        pt_l4->entries[0] = (u64)pt_l3 | PT_FLAGS_RW;
        pt_l3->entries[0] = (0x0ULL | PT_FLAGS_RW | PT_FLAG_PAGESIZE);
}




/* pagetables are the bane of my existence. */
_preinit_
static void init_higher_half(void)
{
        PageTable *pt_l4 = alloc_frame();

        pt_init_temp_identmap(pt_l4);
        set_pagetable(pt_l4);
        pt_init_kcode(pt_l4);
        pt_init_dram_map(pt_l4);
}




_preinit_
static void buddy_handoff(void)
{
        MASTER_BUDDY.total_ram = preinit_pfa.total_ram;
        MASTER_BUDDY.usable_ram = preinit_pfa.usable_ram;

        for (usize i = 0; i < preinit_pfa.region_count; i++) {
                physmem_pregion_t *r = preinit_pfa.region[i];

                void *newbase = phys_to_virt_pm(r->base);
                usize len = r->cur_frames * 4096;

                pi_memset(newbase, 0, sizeof(slave_buddy_t));
                kbud_free_range((phys_addr_t)newbase, len);
        }
}




_preinit_ _noreturn_
void start_kernel(void *mbh)
{
        struct multiboot_tag_mmap *map = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_MMAP);
        multiboot2_tmmap = map;

        alloc_init(map);
        init_higher_half();
        buddy_handoff();
        trampoline();
}

