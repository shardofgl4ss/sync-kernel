//
// Created by SyncShard on 7/25/26.
//

#ifndef KERNEL_PROJECT_IDT_H
#define KERNEL_PROJECT_IDT_H

#include "ktypes.h"

#define FLAGSET(x, f)           (x |= f)
#define FLAGUNSET(x, f)         (x &= ~f)

enum idt_flags {
	F_IDT_GATE_TASK       = 0x5,

	F_IDT_GATE_16BIT_IRQ  = 0x6,
	F_IDT_GATE_16BIT_TRAP = 0x7,

	F_IDT_GATE_IRQ        = 0xE,  // does clear interrupts automatically.
	F_IDT_GATE_TRAP       = 0xF,  // doesn't clear interrupts automatically

	F_IDT_RING_0          = (0 << 5), // kernelspace
	F_IDT_RING_1          = (1 << 5), // unused
	F_IDT_RING_2          = (2 << 5), // unused
	F_IDT_RING_3          = (3 << 5), // userspace

	F_IDT_PRESENT	      = 0x80,
};


extern void idt64_setgate(int interrupt,
                          const void *base,
                          u16 seg_desc,
                          u8 flags);

extern void idt64_enablegate(int interrupt);
extern void idt64_disablegate(int interrupt);
extern void idt_init();


#endif //KERNEL_PROJECT_IDT_H
