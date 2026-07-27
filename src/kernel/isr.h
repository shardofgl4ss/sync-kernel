//
// Created by SyncShard on 7/26/26.
//

#ifndef KERNEL_PROJECT_ISR_H
#define KERNEL_PROJECT_ISR_H

#include "ktypes.h"

typedef struct {
	u64 ds;
	u64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
	u64 vec;
	u64 err;
	u64 rip, cs, rflags, rsp, ss;
} __attribute__((packed)) isr_regs;

extern void x64_isr_init(void);

extern void x64_isr_handler(isr_regs *regs);

#endif //KERNEL_PROJECT_ISR_H
