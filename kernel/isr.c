// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <idt.h>
#include <io.h>
#include <irq.h>
#include <panic.h>
#include <stdio.h>

#include "isr.h"


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
		idt64_setgate((int)i,
		              isr_table[i],
		              (u16)addr,
		              vector_switch_idtflags(i));
		idt64_enablegate((int)i);
	}
}


/* not implemented yet. */
void handler_X86_ISR_DIV(void)
{
	panic("division by zero error!\n");
}
void handler_X86_ISR_DEBUG(void)
{
	panic("debug interrupt\n");
}
void handler_X86_ISR_NMI(void)
{
	panic("received NMI\n");
}
void handler_X86_ISR_BRKP(void)
{
	panic("debug breakpoint\n");
}
void handler_X86_ISR_OVRFLW(void)
{
	panic("overflow error!\n");
}
void handler_X86_ISR_BOUND(void)
{
	panic("bound range error!\n");
}
void handler_X86_ISR_INV_OP(void)
{
	panic("invalid opcode error!\n");
}
void handler_X86_ISR_DEV_NA(void)
{
	panic("device not available!\n");
}
void handler_X86_ISR_DFAULT(void)
{
	panic("double fault!!!\n");
	// x86_cli();
	// for (;;)
	// 	x86_hlt();
}
void handler_X86_ISR_OVERRUN(void)
{
	panic("coprocessor overrun error!\n");
}
void handler_X86_ISR_INV_TSS(void)
{
	panic("invalid TSS error!\n");
}
void handler_X86_ISR_NO_SEG(void)
{
	panic("no segment error!\n");
}
void handler_X86_ISR_STACK(void)
{
	panic("stack error!\n");
}
void handler_X86_ISR_GP(void)
{
	panic("general purpose fault error!\n");
}
void handler_X86_ISR_PAGE_FAULT(void)
{
	panic("page fault error!\n");
}
void handler_X86_ISR_INTEL(void)
{
	panic("mfw intel\n");
}
void handler_X86_ISR_FPUERR(void)
{
	panic("floating point error!\n");
}
void handler_X86_ISR_ALIGNCHK(void)
{
	panic("alignment check error!\n");
}
void handler_X86_ISR_MACHINECHK(void)
{
	panic("machine check!\n");
}
void handler_X86_ISR_AVX_FPE(void)
{
	panic("SSE/AVX floating point exception!\n");
}
void handler_X86_ISR_VIRT(void)
{
	panic("virtualization exception!\n");
}
void handler_X86_ISR_CPE(void)
{
	panic("control protection exception!\n");
}
void handler_X86_ISR_HYPER_INJECT(void)
{
	panic("hypervsior injection\n");
}
void handler_X86_ISR_VM_COMM(void)
{
	panic("VM comm error!\n");
}
void handler_X86_ISR_SECURITY(void)
{
	panic("security exception!\n");
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
                return;
	}

	panic("temporary");
}
