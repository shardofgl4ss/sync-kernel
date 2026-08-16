#include "types.h"
#include "pagetbl.h"

#include <multiboot2.h>


static constexpr int KERN_START_MEMB    = 131072;
static constexpr int PAGE_L1_MEM = sizeof(PageTable) * 512;


#define _PREINIT_               __attribute__((section(".preinit")))


typedef struct multiboot_mmap_entry mapentry_t;
extern _Noreturn void kernel_main(void);


struct kframe_preinit {
        void *base;
        usize offset;
        usize max_len;

        // For now, this is hard coded, i hand calculated
        // it to be 1 u32 for 128KiB. Just change to u64 if
        // KERN_START_MEMB is increased to 256KiB.
        u32 bmap;
} _PAGEALIGNED;


_PREINIT_
struct kframe_preinit *pf = NULL;

extern char _kphys_start[];
extern char _kphys_end[];

extern char KERNEL_OFFSET[];
extern char kstack_top[];

extern char _kheap[];

constexpr uint8_t PT_FLAGS = PT_FLAG_PRESENT | PT_FLAG_WRITABLE;


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
static inline u64 get_koffs(void) {
        __asm__ volatile (
                "movabsq $KERNEL_OFFSET, %0"
                : "=r" (koffs)
        );
}

_PREINIT_
static inline void set_pagetable(PageTable *l4)
{
        __asm__ volatile (
                "mov %0, %%cr3"
                :
                : "r"(l4)
                : "memory"
        );
}


_PREINIT_ __attribute__((naked)) //
_Noreturn static void call_main(struct multiboot_tag_mmap *map)
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
void alloc_preinit(void)
{
        void *heap_phys = (void *)((u64)_kheap - (u64)KERNEL_OFFSET);
        pf = heap_phys;

        u64 *temp = heap_phys;
        for (int i = 0; i < PAGESIZE / sizeof(u64); i++) {
                temp[i] = 0ULL;
        }

        pf->base = (void *)((u64)heap_phys + sizeof(*pf));
        pf->max_len = KERN_START_MEMB;
}




_PREINIT_
void *alloc_frame(void)
{
        if (pf == NULL) 
                return NULL;
        // if preinit memory runs out, something else will die anyway.
        if (pf->offset + PAGESIZE >= pf->max_len)
                die();

        void *a = (void *)((u8 *)pf->base + pf->offset);
        pf->offset += PAGESIZE;

        u64 *temp = a;
        for (int i = 0; i < PAGESIZE / sizeof(u64); i++) {
                temp[i] = 0ULL;
        }

        return a;
}




_PREINIT_
struct multiboot_tag *find_mbt(void *mbh, u32 type)
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
void chk_avail_mem(struct multiboot_tag_mmap *map)
{

        if (map == NULL) die();

        const char *end = (u8 *)map + map->size;
        
        /* The kernel needs at minimum 128kb. starting with less is shit. */
        for (u8 *p = (u8 *)map + sizeof(*map); p < end; p += map->entry_size) {
                const mapentry_t *e = (void *)p;
                const u8 *entry_start = (u8 *)e->addr;
                const u8 *entry_end = (u8 *)e->addr + e->len;

                if (entry_start <= _kphys_start && _kphys_end < entry_end) {
                        if ((u8 *)_kphys_end + KERN_START_MEMB > entry_end) {
                                die();
                        }
                }
        }
}



_PREINIT_
void init_higher_half(void)
{
        const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;
        const u64 kern_l1_count = ((kernsz + (PAGE_L1_MEM - 1)) & ~(PAGE_L1_MEM - 1)) / PAGE_L1_MEM;

        PageTable *l4 = alloc_frame();
        PageTable *l3 = alloc_frame();
        PageTable *l2 = alloc_frame();
        PageTable *l2_low = alloc_frame();
        PageTable *l1 = alloc_frame();

        /* alloc_frame() is contiguous, so just calling it works. for now. */
        for (int i = 0; i < kern_l1_count - 1; i++) {
                alloc_frame();
        }

        u64 koffs = (u64)KERNEL_OFFSET;

        const u64 l4_index = (koffs >> 39) & 0x1ff;
        const u64 l3_index = (koffs >> 30) & 0x1ff;
        const u64 l2_index = (koffs >> 21) & 0x1ff;

        u64 l3_eaddr = ((u64)l3) | PT_FLAGS;
        u64 l2_eaddr = ((u64)l2) | PT_FLAGS;
        u64 l2_low_eaddr = ((u64)l2_low) | PT_FLAGS;

        l4.entries[l4_index] = l3_eaddr;
        l3.entries[l3_index] = l2_eaddr;

        // 2mb low addresses have to be mapped otherwise
        // the moment %cr3 is loaded it will fault.
        l4.entries[0] = l3_eaddr;
        l3.entries[0] = l2_low_eaddr;
        l2_low.entries[0] = (0x0 | PT_FLAGS | 1 << 7);

        for (u64 i = 0; i < PAGE_PT_COUNT; i++) {
                for (u64 j = 0; j < sizeof(PageTable) / sizeof(u64); j++) {
                        u64 frame = ((i * 512) + j) * sizeof(PageTable);
                        l1[i].entries[j] = frame | PT_FLAGS;
                }

                l2.entries[pd_bi + i] = ((u64)(l1[i])) | PT_FLAGS;
        }

        set_pagetable(l4);
}



// reminder: i should probably not make this the kernel init function,
// as it may be in 32 bit mode. atm the kernel is 64-bit only.
_PREINIT_
_Noreturn void start_kernel(void *mbh)
{
        struct multiboot_tag_mmap *map = (void *)find_mbt(mbh, MULTIBOOT_TAG_TYPE_MMAP);

        chk_avail_mem(map);
        alloc_preinit();
        init_higher_half();
        call_main(map);
}

