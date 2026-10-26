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


extern int vm_rb_insert(struct rb_tree *tree, struct vma *vma);
/* Does nothing if vma is NULL. NOTHING! NOTHIIINGNGGG!!! */
extern void vm_rb_remove(struct rb_tree *tree, struct vma *_Nullable vma);
/* Called internally by vm_rb_remove_by_addr() */
extern struct vma *vm_rb_search(struct rb_tree *tree, virt_addr_t addr);
extern void vm_rb_remove_by_addr(struct rb_tree *tree, virt_addr_t addr);
extern int vma_subsystem_init(void);

