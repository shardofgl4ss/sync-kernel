//
// Created by SyncShard on 7/26/26.
//

#include "isr.h"
#include "idt.h"
#include "irq.h"
#include "panic.h"
#include "stdio.h"
#include "vga.h"

extern void *isr_table[256];
extern char CODE_SEGMENT64;


static u8 vector_switch_idtflags(const u8 interrupt)
{
	u8 flag = 0;
	switch (interrupt) {
	default:
		flag = F_IDT_RING_0 | F_IDT_GATE_IRQ;
	}
	return flag;
}


void x64_isr_init(void)
{
	const uintptr_t addr = (uintptr_t)(&CODE_SEGMENT64);
	for (size_t i = 0; i < 256; i++) {
		idt64_setgate(i,
		              isr_table[i],
		              (u16)addr,
		              vector_switch_idtflags(i));
		idt64_enablegate(i);
	}
}


void x64_isr_handler(isr_regs *regs)
{
	vga_setcolor(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK);
	puts("interrupt received while interrupts were disabled, dying.\n");

	panic();
}
