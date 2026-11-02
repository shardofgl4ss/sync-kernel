// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <bootparams.h>
#include <font.h>
#include <framebuffer.h>
#include <kctype.h>
#include <macros.h>
#include <panic.h>
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
        .fb       = NULL,
        .font     = &FONT_PX437_IBM_8X16,
        .color_fg = { .color = KCONSOLE_INIT_COLOR_FG },
        .color_bg = { .color = KCONSOLE_INIT_COLOR_BG },
        .cx       = 0,
        .cy       = 0,
};



static void console_scroll_by(u32 pixels)
{
        fb_ops_t *o = kconsole.fb->ops;
        if (o->scroll) o->scroll(kconsole.fb, pixels);
        kconsole.cy -= pixels;
}



static inline void console_scroll()
{
        console_scroll_by(kconsole.font->pixel_h);
}


/* fills a whole glyph w/h block with the background color.
 * If no glyph is provided, fills font->pixel_w and font->pixel_h */
_hot_
static void console_fill_glyph(const font_glyph_t *_Nullable restrict glyph, u32 x, u32 y)
{
        const kfont_t *restrict font = kconsole.font;
        framebuffer_t *fb = kconsole.fb;

        if (!fb->ops->blit_pixel) return;
        if (!fb->ops->fill_rect) panic("console.c: refactor needed");

        i32 cx = (i32)x;
        i32 cy = (i32)y;

        if (cx < 0 || cy < 0) return;

        i32 g_w;
        i32 g_h;

        if (!glyph) {
                g_w = font->pixel_w;
                g_h = font->pixel_h;
        } else {
                g_w = glyph->width;
                g_h = glyph->height;
        }

        fb->ops->fill_rect(kconsole.fb, cx, cy, g_w, g_h, kconsole.color_bg);
}

/* uses transparent rendering, background is not written. use console_fill_glyph(). */
_hot_
static void console_draw_glyph(const font_glyph_t *restrict glyph, u32 x, u32 y)
{
        const kfont_t *font = kconsole.font;
        framebuffer_t *fb = kconsole.fb;

        if (!fb->ops->blit_pixel) return;
        const i32 by = (i32)y + font->baseline;

        auto const fn_blit = fb->ops->blit_pixel;
        auto const fg = kconsole.color_fg;
        const u8 *bitmap = font->font_bitmap;
        const i32 gly_w = glyph->width;
        const i32 gly_h = glyph->height;
        const u32 base_bit = (u32)glyph->bitmap_offset << 3;

        const i32 dx_base = (i32)x + glyph->x_offset;

        for (i32 h = 0; h < gly_h; h++) {
                i32 dy = by + (i32)glyph->y_offset + h;
                i32 bit_offs = h * gly_w;

                for (i32 w = 0; w < gly_w; w++) {
                        u32 globl_bit = base_bit + bit_offs + w;
                        u32 byte_idx = globl_bit >> 3;
                        u32 bit_pos = 7 - (globl_bit % 8);

                        if ((bitmap[byte_idx] >> bit_pos) & 1) {
                                fn_blit(fb, dx_base + w, dy, fg);
                        }
                }
        }
}


static inline void con_ctrl_carriage_ret(void)
{
        kconsole.cx = 0;
}

static inline void con_ctrl_newl(void)
{
        con_ctrl_carriage_ret();

        if (kconsole.cy + kconsole.font->pixel_h >= kconsole.fb->height)
                console_scroll();
        else
                kconsole.cy += kconsole.font->pixel_h;

        kconsole.cx = 0;
}

static inline void con_ctrl_tab(void)
{
        const font_glyph_t *glyph = &kconsole.font->font_glyphs[0];
        u32 gly_w = glyph->width ? glyph->width : glyph->x_advance;
        if (!gly_w) gly_w = kconsole.font->pixel_w;

        u32 tab_w = gly_w * KCONSOLE_TABSTOP;
        if (!tab_w) return;

        u32 dist = tab_w - (kconsole.cx % tab_w);

        kconsole.cx += dist;

        if (kconsole.cx >= kconsole.fb->width) {
                con_ctrl_newl();
        }
}

static inline void con_ctrl_clear(void)
{
        if (!kconsole.fb->ops->clear) return;
        kconsole.fb->ops->clear(kconsole.fb);
}


static void console_ascii_ctrl(const char c)
{
        switch (c) {
        case '\r':
                con_ctrl_carriage_ret();
                break;
        case '\n':
                con_ctrl_newl();
                break;
        case '\t':
                con_ctrl_tab();
                break;
        default:
                break;
        }
}


static void console_write_cell_at(const char c, u32 x, u32 y)
{
        const kfont_t *font = kconsole.font;

        if (kconsole.cy + kconsole.font->pixel_h >= kconsole.fb->height) {
                con_ctrl_newl();
                y -= kconsole.font->pixel_h;
        }

        if (!isprint(c)) {
                console_ascii_ctrl(c);
                return;
        }

        const font_glyph_t *restrict glyph = &font->font_glyphs[c - ' '];
        console_draw_glyph(glyph, x, y);

        kconsole.cx += glyph->x_advance;
        if (kconsole.cx >= kconsole.fb->width) {
                con_ctrl_newl();
        }
}


void console_setframebuffer(framebuffer_t *fb)
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



void kcon_putchar(const char c)
{
        console_write_cell_at(c, kconsole.cx, kconsole.cy);
}

void kcon_puts(const char *restrict s)
{
        while (*s) kcon_putchar(*s++);
}

void kcon_clear()
{
        con_ctrl_clear();
}


void console_init(void)
{

        framebuffer_t *fb = fbcon_display_init(&KERN_BOOT_INFO);
        console_setframebuffer(fb);
        kcon_puts("SYOS virtualized kernel booting...\n");

        for (int i = 0; i < 1024; i++) {
                for (int j = 0; j < 10'000'000; j++) {
                        __asm__ volatile ("nop");
                }

                if (i % 2 == 0) {
                        kcon_puts("\t1:\tTest string\ttest!\n");
                } else {
                        kcon_puts("\t2:\tstring test\tstring!\n");
                }
        }
}

