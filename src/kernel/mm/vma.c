// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "vma.h"

#include "panic.h"
#include "types.h"
#include "macros.h"
#include "rbtree.h"
#include "slab.h"


// struct address_space {
//
// };

const char *VMA_IDENTIFIER = "vma_struct";

static struct rb_tree vma_tree = {};
static struct kslab_cache vma_cache = {};


static inline struct vma *return_vma_struct(struct rb_node *x)
{
        return substruct_owner(x, struct vma, rb_node);
}




int vm_rb_insert(struct rb_tree *tree, struct vma *vma)
{
        struct rb_node **link = &tree->root;
        struct rb_node *parent = NULL;

        while (*link != NULL) {
                struct rb_node *current = *link;
                struct vma *current_vm = return_vma_struct(current);

                parent = current;

                if (vma->vm_end <= current_vm->vm_start) {
                        link = &current->left;
                } else if (vma->vm_start >= current_vm->vm_end) {
                        link = &current->right;
                } else {
                        return -1;
                }
        }
        
        rb_insert(tree, &vma->rb_node, parent, link);
        return 0;
}




/* Does nothing if vma is NULL. NOTHING! NOTHIIINGNGGG!!! */
void vm_rb_remove(struct rb_tree *vm_tree, struct vma *_Nullable vma)
{
        if (!vma) return;

        rb_remove(vm_tree, &vma->rb_node);
        slab_free(&vma_cache, vma);
}




struct vma *vm_rb_search(struct rb_tree *vm_tree, virt_addr_t addr)
{
        struct rb_node *node = vm_tree->root;
        struct vma *v = NULL;

        while (node) {
                v = return_vma_struct(node);

                if (addr >= v->vm_start && addr < v->vm_end) goto done;
                if (addr < v->vm_start) node = node->left;
                if (addr >= v->vm_end) node = node->right;
        }

        return NULL;
done:
        return v;
}




void vm_rb_remove_by_addr(struct rb_tree *tree, virt_addr_t addr)
{
        // These are separate to help debugging.
        struct vma *v = vm_rb_search(tree, addr);
        vm_rb_remove(tree, v);
}



int vma_subsystem_init(void)
{
        int ret = slabcache_create(&vma_cache, VMA_IDENTIFIER, sizeof(struct vma), alignof(struct vma), 0);
        if (!ret) return 1;
        return 0;
}

