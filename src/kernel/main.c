#include "main.h"

// Reminder: The space where the kernel is located
// only has 640KB of space before colliding with BIOS data.

#include "io.h"
#include "irq.h"
#include "kinit.h"
#include "stdio.h"
#include "gdt.h"
#include "isr.h"
#include "idt.h"
#include "pagetbl.h"
#include "panic.h"

#include "drivers/vga.h"

_Noreturn void kidle(void)
{
	for (;;) {
		x86_hlt();
	}
}

void kernel_main(void *mmap)
{
        kpage_init(mmap);
        kern_gdt_init();
        isr_init();
        idt_init();

	kidle();
}

