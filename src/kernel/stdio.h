// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#ifndef _KERNEL_STDIO_H
#define _KERNEL_STDIO_H

#include "types.h"

extern void putchar(u8 c);
extern void puts(const char *str);

extern void write(const char *str, int len);
extern void clear_line(void);
extern void clear_screen(void);

#endif //_KERNEL_STDIO_H
