// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <types.h>

typedef struct framebuffer framebuffer_t;

typedef struct framebuf_ops {
        void (*blit_pixel)(framebuffer_t *, u32 x, u32 y, u32 color);
        void (*fill_rect)(framebuffer_t *, u32 x, u32 y, u32 w, u32 h, u32 color);
        void (*scroll)(framebuffer_t *, u32 px_rows);
        void (*flush)(framebuffer_t *, u32 x, u32 y, u32 w, u32 h);
} fb_ops_t;



typedef struct framebuffer {
        byte *base;
        u32 width;
        u32 height;
        u32 pitch;
        u8 bpp;
        fb_ops_t *ops;
        void *private;
} framebuffer_t;

