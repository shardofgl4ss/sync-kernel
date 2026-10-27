// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "buddy.h"
#include "macros.h"
#include "rbtree.h"
#include "slab.h"
#include "types.h"

#define VMA_INVALID_PTR ((void *)-1)


typedef enum vma_flags : u32 {
	VM_READ		 = 1 << 0,
	VM_WRITE	 = 1 << 1,
	VM_EXEC		 = 1 << 2,
	VM_USER		 = 1 << 3,
	VM_ANON		 = 1 << 4,
	VM_IO		 = 1 << 5,
	VM_NOCACHE	 = 1 << 6,
	VM_WRITE_THROUGH = 1 << 7,
	VM_SHARED	 = 1 << 8,
	VM_GROWSDOWN	 = 1 << 9,
} vma_flags_t;

typedef struct kernel_vma {
        virt_addr_t vm_start;
        virt_addr_t vm_end;
        page_frame_t **pages;
        vma_flags_t vm_flags;
        struct rb_node rb_node;
} kernel_vma_t;

typedef struct vma_region {
        virt_addr_t r_start;
        virt_addr_t r_end;
        struct rb_tree r_tree;
        // REMINDER: add spinlock to this struct when synchronization primitives are a thing.
} vma_region_t;


extern vma_region_t vma_tree;
extern kslab_cache_t vma_cache;

extern int vma_insert(vma_region_t *vm_tree, kernel_vma_t *vma);
/* Does nothing if vma is NULL. NOTHING! NOTHIIINGNGGG!!! */
extern void vma_remove(vma_region_t *vm_tree, kernel_vma_t *_Nullable vma);
/* Called internally by vm_rb_remove_by_addr() */
extern kernel_vma_t *vma_search(vma_region_t *vm_tree, virt_addr_t addr);
extern void vma_remove_by_addr(vma_region_t *vm_tree, virt_addr_t addr);
extern void *vma_alloc_region(void);
extern void vma_free_region(void);
extern int vma_subsystem_init(void);

