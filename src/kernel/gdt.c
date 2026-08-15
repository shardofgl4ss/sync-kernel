#include <stddef.h>

#include "gdt.h"
#include "types.h"


const struct GDT64 KERN_GDT = {
        .null = 0,
        .cs = {
                .lim_lo = 0,
                .zero = 0,
                .zero1 = 0,
                .ppt = KERNEL_CS_PPT,
                .hi_lim_flags = KERNEL_CS_FLAGS,
                .zero2 = 0,
        },
};

const struct GDT64_Descriptor KERN_GDTR = {
        .limit = (u16)(sizeof(struct GDT64) - 1),
        .base = &KERN_GDT,
};


const u64 CODE_SEGMENT64 = offsetof(struct GDT64, cs);


extern void gdt_asm_init(const struct GDT64_Descriptor *gdtr);

void kern_gdt_init(void)
{
        gdt_asm_init(&KERN_GDTR);
}


