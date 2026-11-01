// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <macros.h>

_always_inline_ static inline void x86_cli(void)   { __asm__ volatile ("cli" ::: "memory"); }
_always_inline_ static inline void x86_sti(void)  { __asm__ volatile ("sti" ::: "memory"); }

