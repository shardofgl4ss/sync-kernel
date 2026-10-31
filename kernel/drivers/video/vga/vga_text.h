// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <types.h>
#include <macros.h>

#include "vga.h"

typedef struct vga_char {
	u8 ch;
	u8 color;
} _packed_ vga_cell_t;


/* initializes the vga screen to nothing and sets color to VGA_INIT_COLOR */
extern void vga_text_init(void);

/* sets any text outputted after this is called to fg, bg */
extern void vga_text_setcolor(vga_color_t fg, vga_color_t bg);

/* Writes a whole string to screen */
extern void vga_text_write(const char *s, int len);

/* puts a single character on the screen */
extern void vga_text_putchar(char c);

/* puts a string on the screen */
extern void vga_text_puts(const char *str);

/* clears the entire screen */
extern void vga_text_clear(void);

/* clears the entire line after the cursor */
extern void vga_text_clear_line(void);

/* returns the current color */
extern vga_color_t vga_text_get_color(void);

