// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <boot.h>
#include <bootparams.h>
#include <macros.h>
#include <types.h>

#include "vga_graphics.h"
#include "vga_text.h"


typedef enum vga_mode {
        VGA_MODE_TEXT = 0,
        VGA_MODE_GRAPHICS,
} vga_mode_t;



struct vga_controller {
        volatile byte *vga_addr;
        volatile byte *vga_end_addr;

        usize win_w;
        usize win_h;

        usize win_pitch;
        usize bits_per_px;

        vga_mode_t mode;
        vga_color_t cur_fg;
        vga_color_t cur_bg;
} kvga_ctrl;



static void (*fn_vga_init)();
static void (*fn_vga_setcolor)(vga_color_t fg, vga_color_t bg);
static void (*fn_vga_write)(const char *s, int len);
static void (*fn_vga_putchar)(char);
static void (*fn_vga_puts)(const char *);
static void (*fn_vga_clear)();
static void (*fn_vga_clear_line)();
static vga_color_t (*fn_vga_get_color)();




void vga_init(kboot_info_t *info)
{
        if (info->bootenv == KERN_BOOT_TYPE_BIOS) { 
                kvga_ctrl.mode = VGA_MODE_TEXT;

                fn_vga_setcolor = &vga_text_setcolor;
                fn_vga_write = &vga_text_write;
                fn_vga_putchar = &vga_text_putchar;
                fn_vga_puts = &vga_text_puts;
                fn_vga_clear = &vga_text_clear;
                fn_vga_clear_line = &vga_text_clear_line;
                fn_vga_get_color = &vga_text_get_color;

                fn_vga_init = &vga_text_init;
	} else {
		kvga_ctrl.mode = VGA_MODE_GRAPHICS;

                fn_vga_setcolor = &vga_graphics_setcolor;
                fn_vga_write = &vga_graphics_write;
                fn_vga_putchar = &vga_graphics_putchar;
                fn_vga_puts = &vga_graphics_puts;
                fn_vga_clear = &vga_graphics_clear;
                fn_vga_clear_line = &vga_graphics_clear_line;
                fn_vga_get_color = &vga_graphics_get_color;

                fn_vga_init = &vga_graphics_init;
	}

        fn_vga_init();
        fn_vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* sets any text outputted after this is called to fg, bg */
void vga_setcolor(vga_color_t fg, vga_color_t bg)
{
        fn_vga_setcolor(fg, bg);
}

/* Writes a whole string to screen */
void vga_write(const char *s, int len)
{

        fn_vga_write(s, len);
}

/* puts a single character on the screen */
void vga_putchar(char c)
{
        fn_vga_putchar(c);
}

/* puts a string on the screen */
void vga_puts(const char *str)
{
        fn_vga_puts(str);
}

/* clears the entire screen */
void vga_clear(void)
{
        fn_vga_clear();
}

/* clears the entire line after the cursor */
void vga_clear_line(void)
{
        fn_vga_clear_line();
}

/* returns the current color */
vga_color_t vga_get_color(void)
{
        return fn_vga_get_color();
}


