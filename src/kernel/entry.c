#include "types.h"
#include "pagetbl.h"


#define _PREINIT_PT     __attribute__((section(".pt")))
#define _PREINIT_DT     __attribute__((section(".preinit.data")))


extern _Noreturn void kernel_main(void);


_PREINIT_PT PageTable _PAGE_PML4;
_PREINIT_PT PageTable _PAGE_PDPT;
_PREINIT_PT PageTable _PAGE_PD;
_PREINIT_PT PageTable _PAGE_PD_LOW;
_PREINIT_PT PageTable _PAGE_PT[PAGE_PT_COUNT];

extern char KERNEL_OFFSET;
extern char kstack_top;

constexpr uint8_t PT_FLAGS = PT_FLAG_PRESENT | PT_FLAG_WRITABLE;


__attribute__((section(".preinit"), unused)) //
static inline void debug(void)
{
        __asm__ volatile (
	        "outb %%al, $0xe9"
                :
                : "a"((u8)'A')
        );
}


__attribute__((section(".preinit"), naked)) //
_Noreturn static void call_main(PageTable *pml4)
{
        __asm__ volatile (
                "movq %rdi, %cr3\n\t"
                "movq $kstack_top, %rsp\n\t"
                "andq $-0x10, %rsp\n\t"
                "pushq $0\n\t"

                // we don't need to use the GOT here,
                // but I like the GOT. GOT my beloved.
                "jmp *kernel_main@GOTPCREL(%rip)\n\t"
                "hlt"
        );
}

// reminder: i should probably not make this the kernel init function,
// as it may be in 32 bit mode. atm the kernel is 64-bit only.
__attribute__((section(".preinit"))) //
_Noreturn void start_kernel(u64 load)
{
        u64 koffs;

        __asm__ volatile (
                "movabsq $KERNEL_OFFSET, %0"
                : "=r" (koffs)
        );

        u64 pml4_bi = (koffs >> 39) & 0x1ff;
        u64 pdpt_bi = (koffs >> 30) & 0x1ff;
        u64 pd_bi = (koffs >> 21) & 0x1ff;

        u64 pdpt = ((u64)&_PAGE_PDPT) | PT_FLAGS;
        u64 pd = ((u64)&_PAGE_PD) | PT_FLAGS;
        u64 pdlow = ((u64)&_PAGE_PD_LOW) | PT_FLAGS;

        _PAGE_PML4.entries[pml4_bi] = pdpt;
        _PAGE_PDPT.entries[pdpt_bi] = pd;

        // 2mb low addresses have to be mapped otherwise
        // the moment %cr3 is loaded it will fault.
        _PAGE_PML4.entries[0] = pdpt;
        _PAGE_PDPT.entries[0] = pdlow;
        _PAGE_PD_LOW.entries[0] = (0x0 | PT_FLAGS | 1 << 7);

        for (u64 i = 0; i < PAGE_PT_COUNT; i++) {
                for (u64 j = 0; j < sizeof(PageTable) / sizeof(u64); j++) {
                        u64 frame = ((i * 512) + j) * sizeof(PageTable);
                        _PAGE_PT[i].entries[j] = frame | PT_FLAGS;
                }


                _PAGE_PD.entries[pd_bi + i] = ((u64)(&_PAGE_PT[i])) | PT_FLAGS;
        }
        
        // for now, we will keep the pagetables in lower half identity mapping.
        call_main(&_PAGE_PML4);
}

