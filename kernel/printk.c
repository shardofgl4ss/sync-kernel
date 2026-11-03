// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <io/console.h>
#include <macros.h>
#include <stdio.h>
#include <types.h>

#include "printk.h"


#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

klog_level_t KERN_LOG_LEVEL = KLOG_DEBUG;


_format_(printf, 2, 0)
isize vprintk(klog_level_t lvl, const char *fmt, va_list args)
{
        char buf[1024];

        isize ret = vsprintf(buf, fmt, args);

        if (ret > 0 && lvl <= KERN_LOG_LEVEL)  {
                kcon_write(buf, MIN(ret, (isize)sizeof(buf) - 1));
        }

        return ret;
}


_format_(printf, 2, 3)
isize printk(klog_level_t lvl, const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);

        isize ret = vprintk(lvl, fmt, args);

        va_end(args);
        return ret;
}

