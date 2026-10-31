// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "boot.h"

#include <bootparams.h>
#include <buddy.h>
#include <io.h>
#include <kstring.h>
#include <macros.h>
#include <multiboot2.h>
#include <pagetbl.h>
#include <types.h>

#define ISA_DEBUGCON_PORT       ((u16)0xE9)
#define MB2_TAG_ERR             ((void *)-1)

typedef struct multiboot_mmap_entry mapentry_t;
extern _Noreturn void kernel_main(void);

_preinit_zeroed_
struct preinit_allocator preinit_pfa = {};
_preinit_zeroed_
struct multiboot_tag_mmap *multiboot2_tmmap = NULL;
_preinit_zeroed_
struct multiboot_tag_framebuffer *mbt_framebuf = NULL;

extern char _kphys_start[];
extern char _kphys_end[];

extern char KERNEL_OFFSET[];
extern char kstack_top[];

extern char _kheap[];

/* For some reason GCC likes to use a .bss and not preinit.bss for a function in preinit section. 
 * so we have to use these. they can't be const char *s, they have to be const char s[] */
_preinit_ro_
const char PREINIT_ERR_NULLMAP[] = "map is NULL at alloc_init()";
_preinit_ro_
const char PREINIT_ERR_MEMORY_REGION_CAP[] = "preinit memory regions exceeded capacity";
_preinit_ro_
const char PREINIT_ERR_OOB_ALLOC[] = "alloc_frame_struct out of bounds";
_preinit_ro_
const char PREINIT_ERR_KCODE_REFACTOR[] = "pt_init_kcode() needs some refactoring!";
_preinit_ro_
const char PREINIT_ERR_PDPT_INVALID[] = "pt_l3 is an invalid ptr!!";


static constexpr uint8_t PT_FLAGS_RW = PT_FLAG_PRESENT | PT_FLAG_WRITABLE;


_preinit_ _hot_
static void pi_memset(void *dst, u8 c, usize n)
{
        u8 *dest = dst;
        for (u32 i = 0; i < n; i++) {
                dest[i] = c;
        }
}


_preinit_
static void isa_debugcon_print(const char *str)
{
#if DEBUG
        while (*str) { // sadly we cant use repne outsb, QEMU doesnt like it
                outb(ISA_DEBUGCON_PORT, *(str++));
        }

        outb(ISA_DEBUGCON_PORT, '\n');
#else
        (void)str;
#endif
}


/* errstr is entirely just for any debugger to see. */
_preinit_ _noreturn_
static inline void die(const char *restrict errstr)
{
        isa_debugcon_print(errstr);
        __asm__ volatile ("cli"); 
        for (;;) __asm__ volatile ("hlt");
}




_preinit_
static inline void pi_set_pagetable(PageTable *pml4)
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

        return MB2_TAG_ERR;
}




_preinit_
static void pf_init_region(const usize region, const struct bootparam_map_entry *restrict ent)
{
        const u64 entry_start = ent->base;
        const u64 entry_end   = ent->base + ent->len;
        // const u64 kernel_sz   = (u64)_kphys_end - (u64)_kphys_start;

        void *addr = (void *)ent->base;
        usize len = ent->len;

        if (unlikely(entry_start <= (u64)_kphys_start
                    && entry_end >= (u64)_kphys_end)) {
                const u64 kspan = (u64)_kphys_end - ent->base;
                addr = (void *)((u64)_kphys_end);
                len -= kspan;
                // addr = (u8 *)addr + kernel_sz;
                // len -= kernel_sz;
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




// _preinit_
// static void sort_region_addrs(physmem_pregion_t **r, usize up_to)
// {
//         for (usize i = 1; i < up_to; i++) {
//                 physmem_pregion_t *k = r[i];
//                 usize j = i;
//
//                 while (j > 0 && r[j - 1]->base > k->base) {
//                         r[j] = r[j - 1];
//                         j--;
//                 }
//
//                 r[j] = k;
//         }
// }




_preinit_
static void alloc_init(struct bootparam_map_info *maps)
{
        if (unlikely(maps == NULL)) die(PREINIT_ERR_NULLMAP);

        pi_memset((void *)preinit_pfa.region, (u8)-1, sizeof(preinit_pfa.region));

        u8 region_num = 0;
        for (u8 i = 0; i < maps->count; i++)
        {
                pf_init_region(region_num, &(maps->entries[i]));

                if (++region_num > 64) {
                        die(PREINIT_ERR_MEMORY_REGION_CAP);
                }
        }

        // u8 *begin = (u8 *)map + sizeof(*map);
        // const u8 *end = (u8 *)map + map->size;
        //
        // int regions = 0;
        // for (u8 *p = begin; p < end; p += map->entry_size) {
        //         const mapentry_t *const ent = (void *)p;
        //
        //
        //         if (ent->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
        //                 preinit_pfa.ram_bytes += ent->len;
        //         }
        //
        //         if (ent->type != MULTIBOOT_MEMORY_AVAILABLE) {
        //                 continue;
        //         }
        //
        //         preinit_pfa.ram_bytes += ent->len;
        //
        //         pf_init_region(regions, ent);
        //
        //         // cant remember if this is at the limit or one off. oh well.
        //         if (++regions > PFRAME_MAX_REGIONS - 1) {
        //                 die(PREINIT_ERR_MEMORY_REGION_CAP);
        //         }
        // }
        //
        // sort_region_addrs(preinit_pfa.region, preinit_pfa.region_count);
}




_preinit_ _malloc_ _assume_aligned_(4096)
static void *alloc_frame(void)
{
        physmem_pregion_t **pr = &preinit_pfa.region[0];
        page_frame_t *frame = KPAGE_ERR;

        const uintptr_t pr_addr_end = ((uintptr_t)(*pr)->base) + ((*pr)->max_frames << 12);

        for (; *pr != KPAGE_ERR; pr++) {
                physmem_pregion_t *r = *pr;

                if (r->cur_frames == 0) continue;
                
                frame = r->top;

                if (unlikely((uintptr_t)frame >= pr_addr_end)) {
                        die(PREINIT_ERR_OOB_ALLOC);
                }

                r->top--;
                r->cur_frames--;

                pi_memset(frame, 0, PAGESIZE);
                // preinit_pfa.boot_pagetable_count++;
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
                pagetbl = (void *)PT_REBASE(*pte);
        }

        return pagetbl;
}




_preinit_ _nonnull_(1)
static void pt_init_dram_map(PageTable *const pt_l4, struct bootparam_map_info *info)
{
        void *dram_map_offs = (void *)get_kernel_phmap();

	const usize ramsz = info->ram_max;

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
        // const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;
        const u64 pt_l1_count = (((u64)_kphys_end + PT_L2_ENTRYMASK) & ~PT_L2_ENTRYMASK) / PT_L2_ENTRYSZ;

        if (pt_l1_count > PT_ENTRIES) die(PREINIT_ERR_KCODE_REFACTOR);

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
                u64 *volatile const restrict pd_e = &pt_l2->entries[l2_entry_idx + i];
                PageTable *pt_l1 = chk_pt_alloc(pd_e);

                // *pd_e = (u64)pt_l1 | PT_FLAGS_RW;
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

        if (unlikely(pt_l3 == KPAGE_ERR)) die(PREINIT_ERR_PDPT_INVALID);

        pt_l4->entries[0] = (u64)pt_l3 | PT_FLAGS_RW;
        pt_l3->entries[0] = (0x0ULL | PT_FLAGS_RW | PT_FLAG_PAGESIZE);
}




/* pagetables are the bane of my existence. */
_preinit_
static void init_higher_half(struct bootparam_map_info *restrict info)
{
        PageTable *pt_l4 = alloc_frame();

        pt_init_temp_identmap(pt_l4);
        pi_set_pagetable(pt_l4);
        pt_init_kcode(pt_l4);
        pt_init_dram_map(pt_l4, info);
}




_preinit_
static void buddy_handoff(struct bootparam_map_info *restrict info)
{
        MASTER_BUDDY.total_ram = info->ram_max;
        MASTER_BUDDY.usable_ram = info->ram_usable;

        // The boot region is skipped until coalescence with the later defunct early init pt's.
        for (usize i = 1; i < preinit_pfa.region_count; i++) {
                physmem_pregion_t *r = preinit_pfa.region[i];

                void *newbase = phys_to_virt_pm((phys_addr_t)r->base);
                usize len = r->cur_frames << 12;

                pi_memset(newbase, 0, sizeof(slave_buddy_t));
                kbud_free_range(newbase, len);
        }
}




_preinit_
static void pi_sort_boot_mapentries(struct bootparam_map_entry *maps, usize up_to)
{
        for (usize i = 1; i < up_to; i++) {
                struct bootparam_map_entry k = maps[i];
                usize j = i;

                while (j > 0 && maps[j - 1].base > k.base) {
                        maps[j] = maps[j - 1];
                        j--;
                }

                maps[j] = k;
        }
}




_preinit_
static void pi_loop_mb2_mapentries(struct multiboot_tag_mmap *map, kboot_info_t *info)
{
        struct bootparam_map_info *map_info = &info->maps;

        u8 *begin = (u8 *)map + sizeof(*map);
        const u8 *end = (u8 *)map + map->size;

        for (u8 *p = begin; p < end; p += map->entry_size) {
                const mapentry_t *ent = (void *)p;
                u8 *restrict i;

                if (ent->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
                        i = &map_info->count_reclaimable;
                        map_info->entries_reclaimable[*i].base = ent->addr;
                        map_info->entries_reclaimable[*i].len = ent->len;
                        (*i)++;
                        
                        continue;
                }

                if (ent->type != MULTIBOOT_MEMORY_AVAILABLE)
                        continue;
                
                i = &map_info->count;
                map_info->entries[*i].base = ent->addr;
                map_info->entries[*i].len = ent->len;
                (*i)++;
        }
        
        pi_sort_boot_mapentries(map_info->entries, map_info->count);
        pi_sort_boot_mapentries(map_info->entries_reclaimable, map_info->count_reclaimable);
}




// for now we just use a generic error, if one keeps failing and I can't pinpoint it, ill add one for each.
_preinit_ro_
const char PREINIT_ERR_MB2_TAG[] = "multiboot2 tag could not be found!";

_preinit_
static void pi_parse_mb2(void *mbh, kboot_info_t *info)
{
        struct multiboot_tag_mmap *map = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_MMAP);
        struct multiboot_tag_framebuffer *fb = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);
        struct multiboot_tag_new_acpi *acpi = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_ACPI_NEW);

        struct multiboot_tag_efi64 *restrict efi64 = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_EFI64);
        struct multiboot_tag_efi32 *restrict efi32 = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_EFI32);

        if (map == MB2_TAG_ERR) { die(PREINIT_ERR_MB2_TAG); }
        if (fb == MB2_TAG_ERR) { die(PREINIT_ERR_MB2_TAG); }
        if (acpi == MB2_TAG_ERR) { die(PREINIT_ERR_MB2_TAG); }


        pi_loop_mb2_mapentries(map, info);


        info->bootenv = (efi64 != MB2_TAG_ERR)
                ? KERN_BOOT_TYPE_EFI64
                : (efi32 != MB2_TAG_ERR)
                        ? KERN_BOOT_TYPE_EFI32
                        : KERN_BOOT_TYPE_BIOS;


        struct multiboot_tag_framebuffer_common *fb_common = &fb->common;
        info->fb.virt = 0;
        info->fb.phys = fb_common->framebuffer_addr;
        info->fb.width = fb_common->framebuffer_width;
        info->fb.height = fb_common->framebuffer_height;
        info->fb.bpp = fb_common->framebuffer_bpp;
        info->fb.pitch = fb_common->framebuffer_pitch;
}




_preinit_ _noreturn_
void start_kernel(void *mbh)
{
        kboot_info_t boot_info = {};
        pi_parse_mb2(mbh, &boot_info);
        // struct multiboot_tag_mmap *map = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_MMAP);
        // multiboot2_tmmap = map;
        // mbt_framebuf = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);

        alloc_init(&boot_info.maps);
        init_higher_half(&boot_info.maps);
        buddy_handoff(&boot_info.maps);
        KERN_BOOT_INFO = boot_info;
        trampoline();
}

