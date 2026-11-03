// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <stdarg.h>
#include <types.h>

extern isize vsnprintf(char *buf, usize size, const char *fmt, va_list args);
extern isize snprintf(char *buf, usize size, const char *fmt, ...);

