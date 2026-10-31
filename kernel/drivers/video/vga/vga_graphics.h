// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <types.h>

#include "vga.h"


extern void vga_graphics_init(void);

/* sets any text outputted after this is called to fg, bg */
extern void vga_graphics_setcolor(vga_color_t fg, vga_color_t bg);

/* Writes a whole string to screen */
extern void vga_graphics_write(const char *s, int len);

/* puts a single character on the screen */
extern void vga_graphics_putchar(char c);

/* puts a string on the screen */
extern void vga_graphics_puts(const char *str);

/* clears the entire screen */
extern void vga_graphics_clear(void);

/* clears the entire line after the cursor */
extern void vga_graphics_clear_line(void);

/* returns the current color */
extern vga_color_t vga_graphics_get_color(void);

