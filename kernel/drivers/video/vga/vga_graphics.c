// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <types.h>

#include "vga.h"
#include "vga_graphics.h"




void vga_graphics_init(void)
{

}

/* sets any text outputted after this is called to fg, bg */
void vga_graphics_setcolor(vga_color_t fg, vga_color_t bg)
{
        (void)fg;
        (void)bg;
}

/* Writes a whole string to screen */
void vga_graphics_write(const char *s, int len)
{
        (void)s;
        (void)len;
}

/* puts a single character on the screen */
void vga_graphics_putchar(char c)
{
        (void)c;
}

/* puts a string on the screen */
void vga_graphics_puts(const char *str)
{
        (void)str;
}

/* clears the entire screen */
void vga_graphics_clear(void)
{

}

/* clears the entire line after the cursor */
void vga_graphics_clear_line(void)
{

}

/* returns the current color */
vga_color_t vga_graphics_get_color(void)
{
        return VGA_COLOR_BLACK;
}

