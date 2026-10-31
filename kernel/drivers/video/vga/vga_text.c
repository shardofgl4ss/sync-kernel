// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "vga.h"
#include "vga_text.h"

#include <boot.h>
#include <kctype.h>
#include <kstring.h>
#include <types.h>
#include <vmalloc.h>


// fg | (bg << 4), sets the kernel tty? init color.
static constexpr vga_color_t VGA_INIT_COLOR = (VGA_COLOR_LIGHT_CYAN | (VGA_COLOR_BLACK << 4));
static constexpr int VGA_TABSTOP = 8;

static constexpr int VGA_WIDTH = 80;
static constexpr int VGA_HEIGHT = 25;
static constexpr int VGA_CELLS = VGA_WIDTH * VGA_HEIGHT;

// should only store enough for one line to make it easier.
// static constexpr int VGA_WBUF_SIZE = VGA_WIDTH;
static constexpr int VGA_MAX_STRING_SIZE = VGA_CELLS;

static_assert(VGA_TABSTOP % 2 == 0 && VGA_TABSTOP <= VGA_WIDTH,
               __FILE__ ": VGA_TABSTOP invalid\n");


static struct kvga_term {
	int cy;	// current, not cursor
	int cx;
	int color;
} kvga;	// only vga driver should be able to access this and VGA_OUT.


static volatile vga_cell_t *VGA_OUT;


static inline vga_color_t vga_set_cell_color(const vga_color_t fg,
                                                const vga_color_t bg)
{
	return (fg | (bg << 4));
}


/* this just writes where you specify */
static void vga_write_cell_at(const vga_cell_t cell, const int x, const int y)
{
	const int vga_idx = (y * VGA_WIDTH) + x;
	VGA_OUT[vga_idx] = cell;
}


/* this does the actual positioning logic */
static void vga_write_cell(vga_cell_t cell)
{
	if (isprint(cell.ch)) {
		vga_write_cell_at(cell, kvga.cx, kvga.cy);

		if (++kvga.cx >= VGA_WIDTH) {
			if (++kvga.cy >= VGA_HEIGHT)
				kvga.cy = 0;
			kvga.cx = 0;
		}
		return;
	}

	switch (cell.ch) {
	case '\r':
		kvga.cx = 0;
		break;
	case '\n':
		if (++kvga.cy >= VGA_HEIGHT)
			kvga.cy = 0;
		kvga.cx = 0;

		break;
	case '\t':
		cell.ch = ' ';
		int distance = VGA_TABSTOP - (kvga.cx % VGA_TABSTOP);

		while (distance--) {
			vga_write_cell_at(cell, kvga.cx++, kvga.cy);

			if (kvga.cx >= VGA_WIDTH) {
				if (++kvga.cy >= VGA_HEIGHT) {
					kvga.cy = 0;
				}
				kvga.cx = 0;
			}
		}
	default:
		break;
	}
}





void vga_text_init(void)
{
        VGA_OUT = ioremap(VGA_TEXT_ADDR, VGA_TEXT_ADDR_LEN, VM_FG_MMIO_FLAGS);

	static constexpr vga_cell_t ch = {.ch = ' ', .color = VGA_INIT_COLOR};

	for (int i = 0; i < VGA_CELLS; i++)
		VGA_OUT[i] = ch;
	kvga.color = VGA_INIT_COLOR;
}


void vga_text_setcolor(const vga_color_t fg, const vga_color_t bg)
{
        kvga.color = vga_set_cell_color(fg, bg);
}


void vga_text_write(const char *s, int len)
{
	if (len == 0)
		return;

	vga_cell_t cell;
	cell.color = kvga.color;

	while (len--) {
		cell.ch = (u8)(*(s++));
		vga_write_cell(cell);
	}
}


void vga_text_putchar(const char c)
{
	const vga_cell_t cell = {
		.ch    = c,
		.color = kvga.color
	};

	vga_write_cell(cell);
}


void vga_text_puts(const char *str)
{
	int len = (int)strlen(str);

	if (len >= VGA_MAX_STRING_SIZE) {
		len = VGA_MAX_STRING_SIZE - 1;
	}

	vga_text_write(str, len);
}


void vga_text_clear(void)
{
	const vga_cell_t ch = {
		.ch    = ' ',
		.color = kvga.color
	};

	for (int i = 0; i < VGA_CELLS; i++)
		VGA_OUT[i] = ch;

	kvga.cx = 0;
	kvga.cy = 0;
}


void vga_text_clear_line(void)
{
	const vga_cell_t ch = {
		.ch    = ' ',
		.color = kvga.color
	};

	int vga_idx = (kvga.cy * VGA_WIDTH) + kvga.cx;
	int distance = VGA_WIDTH - kvga.cx;

	while (distance--)
		VGA_OUT[vga_idx++] = ch;
}


vga_color_t vga_text_get_color(void)
{
	return kvga.color;
}


