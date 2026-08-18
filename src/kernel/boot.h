#ifndef _KERNEL_BOOT_H
#define _KERNEL_BOOT_H

#include "types.h"
#include "kmem.h"
#include "multiboot2.h"


struct kframe_preinit {
        struct multiboot_tag_mmap *map;
        void *base;
        page_frame_t *top;
        u64 region_len;
        u64 rem_frames;
        u64 max_frames;
} __attribute__((aligned(64)));

extern struct kframe_preinit preinit_pfa;

#endif
