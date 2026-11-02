// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <bootparams.h>
#include <kctype.h>
#include <font.h>
#include <framebuffer.h>
#include <macros.h>
#include <types.h>

#include "console.h"


_unused_
static constexpr u32 HEX_RED   = RGBA(255, 0, 0, 255);
_unused_
static constexpr u32 HEX_GREEN = RGBA(0, 255, 0, 255);
_unused_
static constexpr u32 HEX_BLUE  = RGBA(0, 0, 255, 255);

_unused_
static constexpr u32 HEX_BLACK = RGBA(0, 0, 0, 255);
_unused_
static constexpr u32 HEX_WHITE = RGBA(255, 255, 255, 255);


struct kernel_console {
        framebuffer_t *restrict fb;
        const kfont_t *font;

        fb_rgba_color_t color_fg;
        fb_rgba_color_t color_bg;

        u32 cx;
        u32 cy;
};

struct kernel_console kconsole = {
        .color_fg = { .color = RGBA(255, 255, 255, 255) },
        .color_bg = { .color = RGBA(0, 0, 0, 255) },
        .cx = 0,
        .cy = 0,
        .font = &FONT_PX437_IBM_8X16,
        .fb = NULL,
};



void console_scroll(u32 pixels)
{
        fb_ops_t *o = kconsole.fb->ops;
        if (o->scroll) o->scroll(kconsole.fb, pixels);
}


_hot_
static void console_draw_glyph_at(const char c, u32 x, u32 y)
{
        if (unlikely(!isprint(c))) return;

        const kfont_t *font = kconsole.font;
        const font_glyph_t *glyph = &font->font_glyphs[c - ' '];

        if (!kconsole.fb->ops->blit_pixel) return;

        const i32 by = (i32)y + 12;

        for (int h = 0; h < glyph->height; h++) {
                for (int w = 0; w < glyph->width; w++) {
                        u32 bit_offs = (h * glyph->width) + w;
                        u32 globl_bit = ((u32)glyph->bitmap_offset << 3) + bit_offs;
                        u32 byte_idx = globl_bit >> 3;
                        u32 bit_pos = 7 - (globl_bit % 8);

                        const bool is_set = (font->font_bitmap[byte_idx] >> bit_pos) & 1;

                        i32 dx = (i32)x + (i32)glyph->x_offset + w;
                        i32 dy = by + (i32)glyph->y_offset + h;

                        if (is_set) {
                                kconsole.fb->ops->blit_pixel(kconsole.fb, dx, dy, kconsole.color_fg);
                        }
                }
        }
        // TODO abstract console drawing from character output

        kconsole.cx += glyph->x_advance;

        if (kconsole.cx >= kconsole.fb->width) {
                kconsole.cy += font->pixel_h;
                kconsole.cx = 0;

                if (kconsole.cy + font->pixel_h >= kconsole.fb->height) {
                        console_scroll(font->pixel_h);
                }
        }
}

static void console_draw_glyph(const char c)
{
        console_draw_glyph_at(c, kconsole.cx, kconsole.cy);
}

static void console_setframebuffer(framebuffer_t *fb)
{
        if (fb) kconsole.fb = fb;
}


void console_setfont(const kfont_t *font)
{
        if (font) kconsole.font = font;
}

void console_setcolor(const fb_rgba_color_t fg, const fb_rgba_color_t bg)
{
        kconsole.color_fg = fg;
        kconsole.color_bg = bg;
}


void console_putc(const char c)
{
        (void)c;
}



// void console_write(const char *s, int len)
// {
//
// }




void console_init(void)
{

        framebuffer_t *fb = fbcon_display_init(&KERN_BOOT_INFO);
        console_setframebuffer(fb);

        for (int c = '!'; c < '~' + 1; c++) {
                console_draw_glyph((char)c);
        }
}

