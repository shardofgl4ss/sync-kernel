// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <types.h>

/*
 * Font-to-array tool used: https://embeddedunfiltered.in/tools/font-to-c.html
 */

typedef struct {
        u16 bitmap_offset;
        u8 width;
        u8 height;
        u8 x_advance;
        i8 x_offset;
        i8 y_offset;
} font_glyph_t;

extern const u8 kfont_ibm3270_16px_bitmap[];
extern const font_glyph_t kfont_ibm3270_16px_glyphs[];

