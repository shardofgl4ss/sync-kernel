// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

#include "types.h"

enum GDT_BITS {
	GDT_PRESENT    = 1 << 7,                /**< Loaded in memory.  */
	GDT_DPL_RING0  = 0,                     /**< kernelspace        */
	GDT_DPL_RING3  = 1 << 5 | 1 << 6,       /**< userspace          */
	GDT_BIT_DESC_S = 1 << 4,                /**< code/data          */
	GDT_ACCESSED   = 1 << 0,                /**< used by cpu        */
};

enum GDT_BITS_DS {
	GDT_BIT_DS_DIRECTION = 1 << 2,
	GDT_BIT_DS_WRITABLE  = 1 << 1,
};
enum GDT_BITS_CS {
	GDT_BIT_CS_EXECUTABLE = 1 << 3,
	GDT_BIT_CS_CONFORMING = 1 << 2,
	GDT_BIT_CS_READABLE   = 1 << 1,
};
enum GDT_BITS_FLAGS {
	GDT_BIT_FLAG_GRANULARITY = 1 << 3,
	GDT_BIT_FLAG_PROT_MODE	 = 1 << 2,
	GDT_BIT_FLAG_LONG_MODE	 = 1 << 1,
	GDT_BIT_FLAG_AVL	 = 1 << 0,
};

static constexpr u8 KERNEL_PPT = GDT_PRESENT | GDT_DPL_RING0 | GDT_BIT_DESC_S;

static constexpr u8 KERNEL_CS_PPT = KERNEL_PPT | GDT_BIT_CS_EXECUTABLE | GDT_BIT_CS_READABLE;
static constexpr u8 KERNEL_DS_PPT = KERNEL_PPT | GDT_BIT_DS_WRITABLE;
static constexpr u8 KERNEL_CS_FLAGS = GDT_BIT_FLAG_LONG_MODE << 4;


struct GDTEntry {
        word lim_lo;

        word zero;
        byte zero1;

        byte ppt;
        byte hi_lim_flags;
        byte zero2;
} __attribute__((packed));
struct GDT64 {
        qword null;

        struct GDTEntry cs;
} __attribute__((packed));

struct GDT64_Descriptor {
        word limit;
        const struct GDT64 *base;
} __attribute__((packed));


extern const struct GDT64 KERN_GDT;
extern const struct GDT64_Descriptor KERN_GDTR;
extern const u64 CODE_SEGMENT64;

extern void kern_gdt_init(void);


#endif // _KERNEL_GDT_H
