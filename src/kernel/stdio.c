//
// Created by SyncShard on 7/27/26.
//

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
