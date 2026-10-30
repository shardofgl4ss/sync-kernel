// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"

typedef struct {
	u64 ds;
	u64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
	u64 interrupt;
	u64 err;
	u64 rip, cs, rflags, rsp, ss;
} __attribute__((packed)) isr_regs;

enum x86_isr_error {
	X86_ISR_DIV          = 0,
	X86_ISR_DEBUG        = 1,     // debug
	X86_ISR_NMI          = 2,     // non-maskable interrupt
	X86_ISR_BRKP         = 3,     // breakpoint
	X86_ISR_OVRFLW       = 4,     // overflow
	X86_ISR_BOUND        = 5,     // bound range exceeded
	X86_ISR_INV_OP       = 6,     // invalid opcode
	X86_ISR_DEV_NA       = 7,     // device not available
	X86_ISR_DFAULT       = 8,     // ERR: double fault
	X86_ISR_OVERRUN      = 9,     // coprocessor segment overrun (reserved)
	X86_ISR_INV_TSS      = 10,    // ERR: invalid TSS
	X86_ISR_NO_SEG       = 11,    // ERR: no segment
	X86_ISR_STACK        = 12,    // ERR: stack fault
	X86_ISR_GP           = 13,    // ERR: general protection fault
	X86_ISR_PAGE_FAULT   = 14,    // ERR: page fault
	X86_ISR_INTEL        = 15,    // reserved by intel
	X86_ISR_FPUERR       = 16,    // x87 FPU error
	X86_ISR_ALIGNCHK     = 17,    // ERR: alignment chk
	X86_ISR_MACHINECHK   = 18,    // machine chk
	X86_ISR_AVX_FPE      = 19,    // SIMD/AVX floating point exception
	X86_ISR_VIRT         = 20,    // virtualization error
	X86_ISR_CPE          = 21,    // control protection exception
	X86_ISR_HYPER_INJECT = 28,    // hypervisor injection
	X86_ISR_VM_COMM      = 29,    // ERR: VMM comm
	X86_ISR_SECURITY     = 30,    // security exception
};




extern void isr_init(void);

extern void x64_isr_handler(const isr_regs *regs);

