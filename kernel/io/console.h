// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <font.h>
#include <framebuffer.h>
#include <types.h>

#define RGBA(r, g, b, a)        ((u32)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))

extern void console_init(void);

