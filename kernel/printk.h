// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <stdarg.h>
#include <types.h>


typedef enum kernel_log_level {
        KLOG_LEVEL_ERR = 0,
        KLOG_LEVEL_WARN = 1,
        KLOG_LEVEL_INFO = 2,
        KLOG_LEVEL_DEBUG = 3,
} klog_level_t;


extern klog_level_t KERN_LOG_LEVEL;

extern isize vprintk(klog_level_t lvl, const char *buf, va_list args);
extern isize printk(klog_level_t lvl, const char *buf);

