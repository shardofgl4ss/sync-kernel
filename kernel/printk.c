// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <io/console.h>
#include <macros.h>
#include <types.h>

#include "printk.h"


isize vprintk(klog_level_t lvl, const char *buf, va_list args)
{
        return 0;
}

isize printk(klog_level_t lvl, const char *buf)
{
        return 0;
}

