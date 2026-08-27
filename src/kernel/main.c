// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "kstring.h"
#include "io.h"
#include "irq.h"
#include "pframe.h"
#include "stdio.h"
#include "gdt.h"
#include "isr.h"
#include "idt.h"
#include "pagetbl.h"
#include "panic.h"
#include "macros.h"

#include "drivers/vga.h"

_noreturn_ _noinline_
void kidle(void)
{
	for (;;) {
		x86_hlt();
	}
}

_noreturn_ _noinline_
void kernel_main(void)
{
        kalloc_init();
        kern_gdt_init();
        isr_init();
        idt_init();

	kidle();
}

