// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <bootparams.h>
#include <font.h>
#include <framebuffer.h>
#include <kctype.h>
#include <kstring.h>
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

        /* from the font, to avoid having to go through a ptr every reference. */
        i32 cell_w;
        i32 cell_h;
        /* from the fb, to avoid having to go through a ptr every reference. */
        u32 fb_w;
        u32 fb_h;
};

struct kernel_console kconsole = {
        .color_fg = { .color = KCONSOLE_INIT_COLOR_FG },
        .color_bg = { .color = KCONSOLE_INIT_COLOR_BG },
};



static void con_scroll_by(const u32 pixels)
{
        fb_ops_t *o = kconsole.fb->ops;
        if (likely(o->scroll)) o->scroll(kconsole.fb, pixels);
        kconsole.cy -= pixels;
}



static inline void con_scroll()
{
        con_scroll_by(kconsole.cell_h);
}


/* fills a whole glyph w/h block with the background color.
 * If no glyph is provided, fills cell_w and cell_h */
_hot_
static void con_fill_glyph(const font_glyph_t *_Nullable restrict glyph, u32 x, u32 y)
{
        framebuffer_t *fb = kconsole.fb;

        if (unlikely(!fb->ops->blit_pixel)) return;
        if (unlikely(!fb->ops->fill_rect)) panic("console.c: refactor needed");

        i32 cx = (i32)x;
        i32 cy = (i32)y;

        if (cx < 0 || cy < 0) return;

        i32 g_w;
        i32 g_h;

        if (!glyph) {
                g_w = kconsole.cell_w;
                g_h = kconsole.cell_h;
        } else {
                g_w = glyph->width;
                g_h = glyph->height;
        }

        fb->ops->fill_rect(kconsole.fb, cx, cy, g_w, g_h, kconsole.color_bg);
}

/* uses transparent rendering, background is not written. use console_fill_glyph(). */
_hot_
static void con_draw_glyph(const font_glyph_t *restrict glyph, u32 x, u32 y)
{
        const kfont_t *font = kconsole.font;
        framebuffer_t *fb = kconsole.fb;

        if (unlikely(!fb->ops->blit_pixel)) return;
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

        if (kconsole.cy + kconsole.cell_h >= kconsole.fb_h)
                con_scroll();
        else
                kconsole.cy += kconsole.cell_h;

        kconsole.cx = 0;
}

static inline void con_ctrl_tab(void)
{
        const font_glyph_t *glyph = &kconsole.font->font_glyphs[0];
        u32 gly_w = glyph->width ? glyph->width : glyph->x_advance;
        if (!gly_w) gly_w = kconsole.cell_w;

        u32 tab_w = gly_w * KCONSOLE_TABSTOP;
        if (!tab_w) return;

        u32 dist = tab_w - (kconsole.cx % tab_w);

        kconsole.cx += dist;

        if (kconsole.cx >= kconsole.fb_w) {
                con_ctrl_newl();
        }
}

static inline void con_ctrl_backspace(void)
{
        const font_glyph_t *glyph = &kconsole.font->font_glyphs[0];
        u32 gly_w = glyph->width ? glyph->width : glyph->x_advance;
        if (!gly_w) gly_w = kconsole.cell_w;

        if (kconsole.cx >= gly_w) {
                kconsole.cx -= gly_w;
        } else if (kconsole.cy >= (u32)kconsole.cell_h) {
                kconsole.cy -= kconsole.cell_h;
                kconsole.cx = kconsole.fb_w - gly_w;
        } else {
                return;
        }

        con_fill_glyph(glyph, kconsole.cx, kconsole.cy);
}

static inline void con_ctrl_clear(void)
{
        if (unlikely(!kconsole.fb->ops->clear)) return;
        kconsole.fb->ops->clear(kconsole.fb);

        kconsole.cx = 0;
        kconsole.cy = 0;
}


static bool con_ascii_ctrl(const u8 c)
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
        case '\b':
                con_ctrl_backspace();
                break;
        default:
                return false;
        }
        return true;
}


isize kcon_write(const char *restrict buf, usize len)
{
        const kfont_t *font = kconsole.font;
        u8 c;

        if (unlikely(!buf)) return -1;

        isize consumed = 0;

        for (usize i = 0; i < len; i++) {
                c = (u8)buf[i];

                if (kconsole.cy >= kconsole.fb_h - kconsole.cell_h)
                        con_ctrl_newl();
                if (c < ' ' || c > '~') {
                        consumed += con_ascii_ctrl(c);
                        continue;
                }

                const font_glyph_t *restrict glyph = &font->font_glyphs[c - ' '];
                con_draw_glyph(glyph, kconsole.cx, kconsole.cy);
                consumed++;

                kconsole.cx += glyph->x_advance;
                if (kconsole.cx >= kconsole.fb_w)
                        con_ctrl_newl();
        }

        return consumed;
}


void kcon_setframebuffer(framebuffer_t *fb)
{
        if (unlikely(!fb)) return;
        kconsole.fb = fb;
        kconsole.fb_w = fb->width;
        kconsole.fb_h = fb->height;
}

void kcon_setfont(const kfont_t *font)
{
        if (unlikely(!font)) return;
        kconsole.font = font;
        kconsole.cell_w = font->pixel_w;
        kconsole.cell_h = font->pixel_h;
}

void kcon_setcolor(const fb_rgba_color_t fg, const fb_rgba_color_t bg)
{
        kconsole.color_fg = fg;
        kconsole.color_bg = bg;
}



void kcon_putchar(const char c)
{
        kcon_write(&c, 1);
}

void kcon_puts(const char *restrict s)
{
        usize len = strlen(s);

        kcon_write(s, len);
}

void kcon_clear()
{
        con_ctrl_clear();
}


void kcon_init(void)
{

        framebuffer_t *fb = fbcon_display_init(&KERN_BOOT_INFO);
        kcon_setframebuffer(fb);
        kcon_setfont(&FONT_IBM_3270_16PX);
        kcon_puts("SYOS virtualized kernel booting...\n");
}

