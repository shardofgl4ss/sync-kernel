#include "boot.h"

#include "macros.h"
#include "types.h"
#include "pagetbl.h"
#include "kmem.h"

#include <multiboot2.h>


#define _PREINIT_               __attribute__((section(".preinit")))
#define _PREINIT_DATA_          __attribute__((section(".preinit.data")))
#define _PREINIT_BSS_           __attribute__((section(".preinit.bss")))


typedef struct multiboot_mmap_entry mapentry_t;
extern _Noreturn void kernel_main(void);

_PREINIT_BSS_
struct kframe_preinit preinit_pfa = {};

extern char _kphys_start[];
extern char _kphys_end[];

extern char KERNEL_OFFSET[];
extern char kstack_top[];

extern char _kheap[];

static constexpr uint8_t PT_FLAGS = PT_FLAG_PRESENT | PT_FLAG_WRITABLE;


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
_Noreturn static void call_main()
{
	__asm__ volatile (
                "movq $kstack_top, %rsp\n\t"
                "andq $-0x10, %rsp\n\t"
                "pushq $0\n\t"

                // we don't need to use the GOT here,
                // but I like the GOT. GOT my beloved.
                "jmp *kernel_main@GOTPCREL(%rip)\n\t"
                "hlt"
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
static void find_memmap(struct multiboot_tag_mmap *map)
{
        static constexpr u64 highest_alloc_region = 1024 * 1024 * 1024;

        if (unlikely(map == NULL)) die();

        const u8 *end = (u8 *)map + map->size;
        const mapentry_t *e = NULL;
        /* The kernel needs at minimum 128kb. starting with less is shit. */
        for (u8 *p = (u8 *)map + sizeof(*map); p < end; p += map->entry_size) {
                e = (void *)p;
                const u8 *entry_start = (u8 *)e->addr;
                const u8 *entry_end = (u8 *)e->addr + e->len;

                if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
                        continue;

                if (entry_end - entry_start < KERN_START_MEMB)
                        continue;

                // to make sure the preinit allocator region
                // is within the 1GiB identity mapping at 0x0.
                if ((u64)entry_start > highest_alloc_region || (u64)entry_end > highest_alloc_region)
                        continue;

                goto found_map;
        }
        die();

found_map:
        u64 ksize = (u64)_kphys_end - (u64)_kphys_start;

        preinit_pfa.map = map;
        if (e->addr <= (u64)_kphys_start
            && e->addr + e->len >= (u64)_kphys_end)
        {
                preinit_pfa.max_frames = (e->len - ksize) / PAGESIZE;
                preinit_pfa.base = (void *)(e->addr + ksize);
	} 
        else 
        {
                preinit_pfa.max_frames = e->len / PAGESIZE;
                preinit_pfa.base = (void *)e->addr;
        }
        preinit_pfa.rem_frames = preinit_pfa.max_frames;
}


_PREINIT_
static void alloc_preinit(void)
{
        page_frame_t *cur = preinit_pfa.base;
        page_frame_t *prev = NULL;

        for (u64 i = 0; i < preinit_pfa.rem_frames - 1; i++) {
                pi_memset(cur, 1, PAGESIZE);
                cur->next = prev;
                prev = cur;
                cur++;
        }
        pi_memset(cur, 1, PAGESIZE);
        cur->next = prev;

        preinit_pfa.top = cur;
}




_PREINIT_
static void *alloc_frame(void)
{
        if (unlikely(!preinit_pfa.top))
                return NULL;

        page_frame_t *p = preinit_pfa.top;
        preinit_pfa.top = preinit_pfa.top->next;
        pi_memset(p, 0, PAGESIZE);
        preinit_pfa.rem_frames--;

        return p;
}




/* pagetables are the bane of my existence. */
_PREINIT_
static void init_higher_half(void)
{
        constexpr usize PT_TOTAL_MEM = 1024 * 1024 * 2;
        const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;
	const u64 kern_pt_count =
			((kernsz + (PT_TOTAL_MEM - 1)) & ~(PT_TOTAL_MEM - 1)) / PT_TOTAL_MEM;

	PageTable *pml4 = alloc_frame();
        PageTable *pdpt = alloc_frame();
        PageTable *pd = alloc_frame();
        PageTable *pt[kern_pt_count];

        for (usize i = 0; i < kern_pt_count; i++) {
                pt[i] = alloc_frame();
        }

        /* the kernel is linear mapped, so no special math here.        *
         * makes it very easy to just do phys + KERNEL_OFFSET = virt.   */
        const u64 koffs  = (u64)KERNEL_OFFSET;
        const u64 pml4_i = (koffs >> 39) & 0x1ff;
        const u64 pdpt_i = (koffs >> 30) & 0x1ff;
        const u64 pd_i   = (koffs >> 21) & 0x1ff;

        pml4->entries[pml4_i] = (u64)pdpt | PT_FLAGS;
        pdpt->entries[pdpt_i] = (u64)pd | PT_FLAGS;

        /* 1gb identity map for 0x0. makes it simpler to remove later too. */
        pml4->entries[0] = (u64)pdpt | PT_FLAGS;
        pdpt->entries[0] = (0x0ULL | PT_FLAGS | PT_FLAG_PAGESIZE);

        static constexpr usize PT_ENTRIES = sizeof(PageTable) / sizeof(u64);

        u64 base = 0x0;
        for (usize i = 0; i < kern_pt_count; i++) {
                pd->entries[pd_i + i] = (u64)pt[i] | PT_FLAGS;
                PageTable *ptp = pt[i];

                for (usize j = 0; j < PT_ENTRIES; j++) {
                        ptp->entries[j] = base | PT_FLAGS;
                        base += sizeof(PageTable);
                }
        }

        set_pagetable(pml4);
}


_PREINIT_
_Noreturn void start_kernel(void *mbh)
{
        struct multiboot_tag_mmap *map = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_MMAP);

        find_memmap(map);
        alloc_preinit();
        init_higher_half();
        call_main();
}

