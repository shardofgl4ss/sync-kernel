// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <bootparams.h>
#include <macros.h>
#include <types.h>



typedef struct {
        union {
                u32 color;

                struct {
                        u8 red;
                        u8 green;
                        u8 blue;
                        u8 alpha;
                } _packed_;

        } _packed_;
} fb_rgba_color_t;

typedef struct framebuffer framebuffer_t;

typedef struct framebuf_ops {
        void (*blit_pixel)(framebuffer_t *, i32 x, i32 y, fb_rgba_color_t color);
        void (*fill_rect)(framebuffer_t *, i32 x, i32 y, u32 w, u32 h, fb_rgba_color_t color);
        void (*scroll)(framebuffer_t *, u32 px_rows);
        void (*flush)(framebuffer_t *, i32 x, i32 y, u32 w, u32 h);
        void (*clear)(framebuffer_t *);
} fb_ops_t;



typedef struct framebuffer {
        volatile u32 *base;
        u32 width;
        u32 height;
        u32 pitch;
        u8 bpp;
        fb_ops_t *ops;
        void *private;
} framebuffer_t;


extern framebuffer_t *fbcon_display_init(kboot_info_t *boot_info);

