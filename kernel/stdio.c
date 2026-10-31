// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "stdio.h"
#include "types.h"

// These are just wrappers to more easily swap them if needed later on.

void putchar(u8 c)
{
        (void)c;
}


void puts(const char *str)
{
        (void)str;
}


void write(const char *str, const int len)
{
        (void)str;
        (void)len;
}


void clear_line(void)
{
}


void clear_screen(void)
{
}
