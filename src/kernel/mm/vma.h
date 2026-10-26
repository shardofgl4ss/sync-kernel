// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "types.h"
#include "rbtree.h"

#define _Nullable
#define _Nonnull

typedef uintptr_t virt_addr_t;

struct vma {
        virt_addr_t vm_start;
        virt_addr_t vm_end;
        u32 vm_flags;
        struct rb_node rb_node;
};

extern struct rb_tree vma_tree;
extern struct kslab_cache vma_cache;

extern int vm_area_insert(struct rb_tree *vm_tree, struct vma *vm_area);
/* Does nothing if vma is NULL. NOTHING! NOTHIIINGNGGG!!! */
extern void vm_area_remove(struct rb_tree *vm_tree, struct vma *_Nullable vm_area);
/* Called internally by vm_rb_remove_by_addr() */
extern struct vma *vm_area_search(struct rb_tree *vm_tree, virt_addr_t addr);
extern void vm_area_remove_by_addr(struct rb_tree *vm_tree, virt_addr_t addr);
extern int vma_subsystem_init(void);

