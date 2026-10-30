// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"
#include "vma.h"

static constexpr vma_flags_t VM_FG_RW = VM_READ | VM_WRITE;
static constexpr vma_flags_t VM_FG_CODE = VM_READ | VM_EXEC;
static constexpr vma_flags_t VM_FG_HEAP = VM_READ | VM_WRITE | VM_ANON;
static constexpr vma_flags_t VM_FG_STACK = VM_READ | VM_WRITE | VM_ANON | VM_GROWSDOWN;
static constexpr vma_flags_t VM_FG_MMIO_FLAGS = VM_FG_RW | VM_IO;

extern int vmalloc_init(void);

extern void *ioremap(phys_addr_t phy, usize len, vma_flags_t flags);
extern void *ioremap_prot(phys_addr_t phy, usize len, vma_flags_t flags);
extern void *iounmap(void *vaddr);
/* singla page vmapping */
extern void *kmap(phys_addr_t phy);
extern void kunmap(void *vaddr);
