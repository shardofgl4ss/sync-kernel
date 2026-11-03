// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#define SYOS_VERSION    0
#define SYOS_SUBVERSION 1

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
#include <printk.h>
#include <types.h>

#include <io/console.h>
#include <mm/kmalloc.h>


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
        cpu_get_capabilities();
        gdt_init();
        mm_init();
        kcon_init();
        isr_init();
        idt_init();
        printk(KLOG_DEBUG, "SYOS kernel v%d.%d --- Booted, Idle Reached\n", SYOS_VERSION, SYOS_SUBVERSION);
	kidle();
}

