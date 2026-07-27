//
// Created by SyncShard on 7/25/26.
//

#include "vga.h"
#include "kstring.h"
#include "ktypes.h"

#define VGA_WIDTH	80
#define VGA_HEIGHT	25
#define VGA_MEMORY	0xb8000

struct vga_terminal {
	size_t term_row;
	size_t term_col;
	u8 term_color;
	volatile u16 *term_buffer;
};

__attribute__((pure))
static inline u8 vga_entry_color(const enum vga_color fg, const enum vga_color bg)
{
	return fg | bg << 4;
}

__attribute__((pure))
static inline u16 vga_entry(const u8 c, const u8 color)
{
	return (u16)c | (u16)color << 8;
}

 static struct vga_terminal terminal = {
	.term_row = 0,
	.term_col = 0,
	.term_color = 0,
	.term_buffer = (u16 *)VGA_MEMORY,
};


void terminal_init(void)
{
	terminal.term_color = vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t i = y * VGA_WIDTH + x;
			terminal.term_buffer[i] = vga_entry(' ', terminal.term_color);
		}
	}
}

void terminal_setcolor(const u8 fg, const u8 bg)
{
	terminal.term_color = vga_entry_color(fg, bg);
}

void terminal_put_entry_at(const char c, const u8 color, const size_t x, const size_t y)
{
	const size_t i = y * VGA_WIDTH + x;
	terminal.term_buffer[i] = vga_entry(c, color);
}

void terminal_putc(const char c)
{
	switch (c) {
	case '\r':
		terminal.term_col = 0;
		return;
	case '\n':
		terminal.term_col = 0;
		if (++terminal.term_row == VGA_HEIGHT)
			terminal.term_row = 0;
		return;
	default:
		break;
	}
	terminal_put_entry_at(c, terminal.term_color, terminal.term_col, terminal.term_row);

	if (++terminal.term_col == VGA_WIDTH) {
		terminal.term_col = 0;
		if (++terminal.term_row == VGA_HEIGHT)
			terminal.term_row = 0;
	}
}

void terminal_puts(const char *restrict data)
{
	for (size_t i = 0; i < strlen(data); i++)
		terminal_putc(data[i]);
}


void terminal_clear(void)
{
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t i = y * VGA_WIDTH + x;
			terminal.term_buffer[i] = vga_entry(' ', terminal.term_color);
		}
	}
}

