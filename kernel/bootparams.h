// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include <types.h>

struct bootparam_framebuf_info;
struct bootparam_map_info;
struct bootparam_map_entry;

typedef enum kern_boot_type : u8 {
        KERN_BOOT_TYPE_BIOS = 0,
        KERN_BOOT_TYPE_EFI32,
        KERN_BOOT_TYPE_EFI64,
} kern_boot_type_t;


typedef struct {
        char cmdline[256];

        struct bootparam_framebuf_info {
                phys_addr_t phys;
                virt_addr_t virt;
                u32 width, height;
                u32 pitch;
                u8 bpp;
        } fb;

        struct bootparam_map_info {
                struct bootparam_map_entry {
                        u64 base;
                        u64 len;
                } entries[64];

                /* reclaimable entries specifically refer to ACPI reclaimable. */

                struct bootparam_map_entry {
                        u64 base;
                        u64 len;
                } entries_reclaimable[8];

                u64 ram_max;
                u64 ram_usable;

                u8 count;
                u8 count_reclaimable;
        } maps;

        u64 acpi_rsdp_addr;

        kern_boot_type_t bootenv;
} kboot_info_t;

extern kboot_info_t KERN_BOOT_INFO;

