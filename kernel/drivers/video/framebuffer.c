// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include <bootparams.h>
#include <font.h>
#include <gop/gop.h>
#include <macros.h>
#include <mm/vmalloc.h>
#include <panic.h>
#include <types.h>

#include "framebuffer.h"


static framebuffer_t kernel_framebuffer = {};


framebuffer_t *fbcon_display_init(kboot_info_t *boot_info)
{
        // for now we will just assume BIOS means VESA, UEFI means GOP.
        struct bootparam_framebuf_info *info = &boot_info->fb;

        if ((void *)info->phys == nullptr) {
                // TODO vga text
        }

        vma_flags_t flags = VM_FG_MMIO_FLAGS | VM_WRITE_THROUGH | VM_NOCACHE;

        void (*fn_fb_init)(framebuffer_t *) = nullptr;

        if (boot_info->bootenv > KERN_BOOT_TYPE_BIOS) {
                // UEFI GOP requires caching.
                flags &= ~VM_NOCACHE;
                fn_fb_init = &gop_fb_init;
        } else {
                // fn_fb_init = &vesa_fb_init;
        }

        kernel_framebuffer.height = info->height;
        kernel_framebuffer.width = info->width;
        kernel_framebuffer.bpp = info->bpp;
        kernel_framebuffer.pitch = info->pitch;

        virt_addr_t addr = (virt_addr_t)ioremap(info->phys, info->height * info->pitch, flags);
        if (!addr) panic("ioremap fbcon failure!");

        kernel_framebuffer.base = (volatile u32 *)addr;
        info->virt = addr;

        if (fn_fb_init != nullptr) fn_fb_init(&kernel_framebuffer);

        return &kernel_framebuffer;
}

