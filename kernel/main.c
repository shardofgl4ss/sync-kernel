// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <cpu_feats.h>
#include <gdt.h>
#include <idt.h>
#include <io.h>
#include <irq.h>
#include <isr.h>
#include <kstring.h>
#include <macros.h>
#include <pagetbl.h>
#include <panic.h>
#include <types.h>

#include <io/console.h>
#include <mm/buddy.h>
#include <mm/kmalloc.h>
#include <mm/slab.h>
#include <mm/vma.h>
#include <mm/vmalloc.h>
#include <vga/vga.h>


_noreturn_
void kidle(void)
{
        kcon_puts("kernel reached idle!\n");
	for (;;) {
		x86_hlt();
	}
}

_noreturn_
void kernel_main(void)
{
        cpu_get_capabilities();
        mm_init();
        kcon_init();
        gdt_init();
        isr_init();
        idt_init();
	kidle();
}

