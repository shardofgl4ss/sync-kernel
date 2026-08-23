// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "panic.h"

#include "io.h"
#include "irq.h"
#include "stdio.h"
#include "drivers/vga.h"


_Noreturn void panic(char *panicmsg)
{
	vga_setcolor(VGA_COLOR_RED, VGA_COLOR_BLACK);
	puts(panicmsg);
	puts("\n");
	puts("\n----- KERNEL PANIC -----\n");
	x86_cli();
	for (;;) {
		x86_hlt();
	}
}
