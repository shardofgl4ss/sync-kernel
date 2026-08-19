#include "main.h"

#include "io.h"
#include "irq.h"
#include "stdio.h"
#include "gdt.h"
#include "isr.h"
#include "idt.h"
#include "pagetbl.h"
#include "panic.h"
#include "kmem.h"

#include "drivers/vga.h"

_Noreturn void kidle(void)
{
	for (;;) {
		x86_hlt();
	}
}

_Noreturn void kernel_main(void)
{
        kalloc_init();
        kern_gdt_init();
        isr_init();
        idt_init();

	kidle();
}

