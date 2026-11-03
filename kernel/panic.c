// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <io.h>
#include <irq.h>
#include <macros.h>
#include <printk.h>

#include "panic.h"


_noreturn_
void panic(char *panicmsg)
{
	// vga_setcolor(VGA_COLOR_RED, VGA_COLOR_BLACK);

	printk(0, panicmsg);
	printk(0, "\n");
	printk(0, "\n----- KERNEL PANIC -----\n");
	x86_cli();
	for (;;) {
		x86_hlt();
	}
}
