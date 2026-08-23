// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard
//
#ifndef _KERNEL_BOOT_H
#define _KERNEL_BOOT_H

#include "types.h"
#include "pframe.h"
#include "multiboot2.h"


extern struct kpage_core preinit_pfa;
extern struct multiboot_tag_mmap *multiboot2_tmmap;

#endif

