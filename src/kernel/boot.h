#ifndef _KERNEL_BOOT_H
#define _KERNEL_BOOT_H

#include "types.h"
#include "kmem.h"
#include "multiboot2.h"

typedef struct {
        struct multiboot_tag_mmap *map;
        page_frame_t *top;
        void *base;
        u64 frames;
        u64 addrlen;
} preinit_meminfo_t;

#endif
