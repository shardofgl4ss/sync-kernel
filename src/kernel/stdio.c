// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "stdio.h"
#include "types.h"
#include "vga.h"

// These are just wrappers to more easily swap them if needed later on.

void putchar(u8 c)
{
	vga_putchar(c);
}


void puts(const char *str)
{
	vga_puts(str);
}


void write(const char *str, const int len)
{
	vga_write(str, len);
}


void clear_line(void)
{
	vga_clear_line();
}


void clear_screen(void)
{
	vga_clear();
}
