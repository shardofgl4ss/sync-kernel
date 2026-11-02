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

typedef struct {
        u8 pixel_h;
        u8 pixel_w;
        const u8 *font_bitmap;
        const font_glyph_t *font_glyphs;
} kfont_t;


// Third Party font assets:

// font from: https://github.com/rbanffy/3270font
// license: https://github.com/rbanffy/3270font?tab=License-1-ov-file
extern const kfont_t FONT_IBM_3270_16PX;

// font from: https://github.com/retro-vault/font-vault/
// license: https://github.com/retro-vault/font-vault/tree/main/source/fonts/oldschool_ibm_vga_8x16
extern const kfont_t FONT_PX437_IBM_8X16;

