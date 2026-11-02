// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <font.h>
#include <framebuffer.h>
#include <types.h>


#define RGBA(r, g, b, a)        ((u32)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))
#define KCONSOLE_TABSTOP        8
#define KCONSOLE_INIT_COLOR_FG  RGBA(0, 255, 255, 255)
#define KCONSOLE_INIT_COLOR_BG  RGBA(0, 0, 0, 255)


extern void kcon_clear();
extern void kcon_puts(const char *restrict s);
extern void kcon_putchar(char c);
extern void kcon_setcolor(fb_rgba_color_t fg, fb_rgba_color_t bg);
extern void kcon_setfont(const kfont_t *font);
extern void kcon_setframebuffer(framebuffer_t *fb);
extern void kcon_init(void);;

