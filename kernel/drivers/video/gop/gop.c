// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <framebuffer.h>
#include <kstring.h>
#include <macros.h>
#include <types.h>

#include "gop.h"

struct gop_pixel_24 {
        u8 r;
        u8 g;
        u8 b;
} _packed_;

_hot_
static void gop_blit(framebuffer_t *restrict fb, i32 x, i32 y, fb_rgba_color_t rgba)
{
        if (x < 0 || x >= (i32)fb->width || y < 0 || y >= (i32)fb->height) return;
        volatile u8 *px = ((volatile u8 *)fb->base 
                        + (y * fb->pitch) 
                        + (x * (fb->bpp >> 3)));

	px[0] = rgba.blue;
	px[1] = rgba.green;
        px[2] = rgba.red;
}

static void gop_scroll(framebuffer_t *restrict fb, u32 px_rows)
{
        usize bytes_row = px_rows * fb->pitch;
        usize bytes_total = fb->height * fb->pitch;

        memmove((u8 *)fb->base, (u8 *)fb->base + bytes_row, bytes_total - bytes_row);
        memset((u8 *)fb->base + bytes_total - bytes_row, 0, bytes_row);
}

static void gop_clear(framebuffer_t *restrict fb)
{
        struct gop_pixel_24 col = {};

        // These are mainly to make sure the compiler doesn't
        // recompute these every time. No idea if the compiler
        // already does this or not. It probably does. Oh well.
        const volatile u8 *base = (volatile void *)fb->base;
        const u32 pitch = fb->pitch;
        const u8 bpp = fb->bpp >> 3;
        
        volatile struct gop_pixel_24 *restrict px;
        for (u32 y = 0; y < fb->height; y++) {
                for (u32 x = 0; x < fb->pitch; x++) {
                        px = (volatile struct gop_pixel_24 *)(base + (y * pitch) + (x * bpp));
			*px = col;
                }
        }
}

static void gop_fill_rect(framebuffer_t *restrict fb, i32 x, i32 y, i32 w, i32 h, fb_rgba_color_t color)
{
        if (x >= (i32)fb->width || y >= (i32)fb->height) return;
        if (x + w <= 0 || y + h <= 0) return;

        i32 x2 = x + w;
        i32 y2 = y + h;

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x2 > (i32)fb->width)  x2 = (i32)fb->width;
        if (y2 > (i32)fb->height) y2 = (i32)fb->height;

        w = x2 - x;
        h = y2 - y;

        if (w <= 0 || h <= 0) return;
        u8 *row = (u8 *)fb->base + (y * fb->pitch) + (x * sizeof(struct gop_pixel_24));

        for (i32 i = 0; i < h; i++) {
                volatile struct gop_pixel_24 *px = (void *)row;
                for (i32 j = 0; j < w; j++) {
                        px[j].b = color.blue;
                        px[j].g = color.green;
                        px[j].r = color.red;
                }

                row += fb->pitch;
        }
}



static const fb_ops_t gop_funcs = {
        .blit_pixel = &gop_blit,
        .fill_rect = &gop_fill_rect,
        .clear = &gop_clear,
        .scroll = &gop_scroll,
        .flush = NULL,
};




void gop_fb_init(framebuffer_t *restrict fb)
{
        fb->ops = &gop_funcs;
        fb->private = NULL;
}

