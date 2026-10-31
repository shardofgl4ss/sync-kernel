// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <bootparams.h>
#include <types.h>

typedef enum vga_color : u8 {
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
} vga_color_t;

#include "vga_text.h"

static constexpr uintptr_t VGA_GRAPHICS_ADDR = 0xA000;
static constexpr uintptr_t VGA_TEXT_ADDR = 0xB8000;
static constexpr usize VGA_GRAPHICS_ADDR_LEN = 65536; // might be wrong
static constexpr usize VGA_TEXT_ADDR_LEN = (80 * 25) * sizeof(vga_cell_t);


/* initializes the vga screen to nothing and sets color to VGA_INIT_COLOR */
extern void vga_init(kboot_info_t *info);

/* sets any text outputted after this is called to fg, bg */
extern void vga_setcolor(vga_color_t fg, vga_color_t bg);

/* Writes a whole string to screen */
extern void vga_write(const char *s, int len);

/* puts a single character on the screen */
extern void vga_putchar(char c);

/* puts a string on the screen */
extern void vga_puts(const char *str);

/* clears the entire screen */
extern void vga_clear(void);

/* clears the entire line after the cursor */
extern void vga_clear_line(void);

/* returns the current color */
extern vga_color_t vga_get_color(void);

