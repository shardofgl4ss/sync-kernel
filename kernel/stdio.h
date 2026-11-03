// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <stdarg.h>
#include <types.h>

#define _format_(...)      __attribute__((format(__VA_ARGS__)))


_format_(printf, 2, 0)
extern isize vsprintf(char *buf, const char *fmt, va_list args);
_format_(printf, 2, 3)
extern isize sprintf(char *buf, const char *fmt, ...);

