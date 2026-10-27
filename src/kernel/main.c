// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "irq.h"
#include "isr.h"
#include "kstring.h"
#include "macros.h"
#include "pagetbl.h"
#include "panic.h"
#include "stdio.h"

#include "drivers/vga.h"
#include "mm/buddy.h"
#include "mm/kmalloc.h"
#include "mm/slab.h"
#include "mm/vma.h"
#include "mm/vmalloc.h"

_noreturn_
void kidle(void)
{
	for (;;) {
		x86_hlt();
	}
}

_noreturn_
void kernel_main(void)
{
        mm_init();
        gdt_init();
        isr_init();
        idt_init();

	kidle();
}

