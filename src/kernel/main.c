#include "main.h"

// Reminder: The space where the kernel is located
// only has 640KB of space before colliding with BIOS data.

#include "idt.h"
#include "io.h"
#include "irq.h"
#include "kinit.h"

#include "drivers/vga.h"

extern char _begin;
#define KMAP_ORG        _begin

_Noreturn void kidle(void)
{
	x86_sti();
	for (;;) {
		x86_hlt();
	}
}

void kernel_main(void)
{
	kinit();
	x86_sti();

	terminal_puts("Hello world, from 64 bit C kernel!\n");

	__asm__ volatile ("int3");

	kidle();
}

