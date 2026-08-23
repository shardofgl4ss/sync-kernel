// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#ifndef _KERNEL_VGA_H
#define _KERNEL_VGA_H

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15
};

enum vga_mode {
	STDOUT = 1,
	STDERR = 2 // at the moment, STDERR just makes output red lol
};

/* initializes the vga screen to nothing and sets color to VGA_INIT_COLOR */
extern void vga_init(void);

/* sets any text outputted after this is called to fg, bg */
extern void vga_setcolor(enum vga_color fg, enum vga_color bg);

/* Writes a whole string to screen */
void vga_write(const char *s, int len);

/* puts a single character on the screen */
extern void vga_putchar(char c);

/* puts a string on the screen */
extern void vga_puts(const char *str);

/* clears the entire screen */
extern void vga_clear(void);

/* clears the entire line after the cursor */
extern void vga_clear_line(void);

/* returns the current color */
extern enum vga_color vga_get_color(void);

#endif //_KERNEL_VGA_H
