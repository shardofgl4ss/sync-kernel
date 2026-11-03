// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <stdarg.h>
#include <types.h>

#define _format_(...)      __attribute__((format(__VA_ARGS__)))

typedef enum kernel_log_level {
        KLOG_ERR = 0,
        KLOG_WARN = 1,
        KLOG_INFO = 2,
        KLOG_DEBUG = 3,
} klog_level_t;


extern klog_level_t KERN_LOG_LEVEL;

_format_(printf, 2, 0)
extern isize vprintk(klog_level_t lvl, const char *fmt, va_list args);
_format_(printf, 2, 3)
extern isize printk(klog_level_t lvl, const char *fmt, ...);

