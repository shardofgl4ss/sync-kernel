// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H


__attribute__((always_inline)) static inline void x86_cli(void)   { __asm__ volatile ("cli" ::: "memory"); }
__attribute__((always_inline)) static inline void x86_sti(void)  { __asm__ volatile ("sti" ::: "memory"); }

#endif //_KERNEL_IRQ_H
