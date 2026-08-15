//
// Created by SyncShard on 7/26/26.
//

#include "isr.h"
#include "idt.h"
#include "io.h"
#include "irq.h"
#include "panic.h"
#include "stdio.h"
#include "vga.h"

extern void *isr_table[256];
extern char CODE_SEGMENT64;
typedef void (*isr_handler_table[256])(void);


static u8 vector_switch_idtflags(const u8 interrupt)
{
	u8 flag = 0;
	switch (interrupt) {
	default:
		flag = F_IDT_RING_0 | F_IDT_GATE_IRQ;
	}
	return flag;
}


void isr_init(void)
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


void handler_X86_ISR_DIV(void)
{
	puts("division by zero error!\n");
}
void handler_X86_ISR_DEBUG(void)
{
	puts("debug interrupt\n");
}
void handler_X86_ISR_NMI(void)
{
	puts("received NMI\n");
}
void handler_X86_ISR_BRKP(void)
{
	puts("debug breakpoint\n");
}
void handler_X86_ISR_OVRFLW(void)
{
	puts("overflow error!\n");
}
void handler_X86_ISR_BOUND(void)
{
	puts("bound range error!\n");
}
void handler_X86_ISR_INV_OP(void)
{
	puts("invalid opcode error!\n");
}
void handler_X86_ISR_DEV_NA(void)
{
	puts("device not available!\n");
}
void handler_X86_ISR_DFAULT(void)
{
	puts("double fault!!!\n");
	x86_cli();
	for (;;)
		x86_hlt();
}
void handler_X86_ISR_OVERRUN(void)
{
	puts("coprocessor overrun error!\n");
}
void handler_X86_ISR_INV_TSS(void)
{
	puts("invalid TSS error!\n");
}
void handler_X86_ISR_NO_SEG(void)
{
	puts("no segment error!\n");
}
void handler_X86_ISR_STACK(void)
{
	puts("stack error!\n");
}
void handler_X86_ISR_GP(void)
{
	puts("general purpose fault error!\n");
}
void handler_X86_ISR_PAGE_FAULT(void)
{
	puts("page fault error!\n");
}
void handler_X86_ISR_INTEL(void)
{
	puts("mfw intel\n");
}
void handler_X86_ISR_FPUERR(void)
{
	puts("floating point error!\n");
}
void handler_X86_ISR_ALIGNCHK(void)
{
	puts("alignment check error!\n");
}
void handler_X86_ISR_MACHINECHK(void)
{
	puts("machine check!\n");
}
void handler_X86_ISR_AVX_FPE(void)
{
	puts("SSE/AVX floating point exception!\n");
}
void handler_X86_ISR_VIRT(void)
{
	puts("virtualization exception!\n");
}
void handler_X86_ISR_CPE(void)
{
	puts("control protection exception!\n");
}
void handler_X86_ISR_HYPER_INJECT(void)
{
	puts("hypervsior injection\n");
}
void handler_X86_ISR_VM_COMM(void)
{
	puts("VM comm error!\n");
}
void handler_X86_ISR_SECURITY(void)
{
	puts("security exception!\n");
}


isr_handler_table table = {
	handler_X86_ISR_DIV,
	handler_X86_ISR_DEBUG,
	handler_X86_ISR_NMI,
	handler_X86_ISR_BRKP,
	handler_X86_ISR_OVRFLW,
	handler_X86_ISR_BOUND,
	handler_X86_ISR_INV_OP,
	handler_X86_ISR_DEV_NA,
	handler_X86_ISR_DFAULT,
	handler_X86_ISR_OVERRUN,
	handler_X86_ISR_INV_TSS,
	handler_X86_ISR_NO_SEG,
	handler_X86_ISR_STACK,
	handler_X86_ISR_GP,
	handler_X86_ISR_PAGE_FAULT,
	handler_X86_ISR_INTEL,
	handler_X86_ISR_FPUERR,
	handler_X86_ISR_ALIGNCHK,
	handler_X86_ISR_MACHINECHK,
	handler_X86_ISR_AVX_FPE,
	handler_X86_ISR_VIRT,
	handler_X86_ISR_CPE,
	handler_X86_ISR_HYPER_INJECT,
	handler_X86_ISR_VM_COMM,
	handler_X86_ISR_SECURITY
};


void x64_isr_handler(const isr_regs *restrict regs)
{
	if (table[regs->interrupt]) {
		table[regs->interrupt]();
	}


	vga_setcolor(VGA_COLOR_MAGENTA, VGA_COLOR_BLACK);
	puts("interrupt received while interrupts were disabled, dying.\n");

	panic();
}
