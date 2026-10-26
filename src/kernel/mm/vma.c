// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "vma.h"

#include "panic.h"
#include "types.h"
#include "macros.h"
#include "rbtree.h"
#include "slab.h"


const char *VMA_IDENTIFIER = "kernel_vma_struct";

vma_region_t vma_tree = {};
kslab_cache_t vma_cache = {};


static inline kernel_vma_t *return_vma_struct(struct rb_node *x)
{
        return substruct_owner(x, kernel_vma_t, rb_node);
}




int vma_insert(vma_region_t *vm_tree, kernel_vma_t *vma)
{
        struct rb_node **link = &vm_tree->r_tree.root;
        struct rb_node *parent = NULL;

        while (*link != NULL) {
                struct rb_node *current = *link;
                kernel_vma_t *current_vm = return_vma_struct(current);

                parent = current;

                if (vma->vm_end <= current_vm->vm_start) {
                        link = &current->left;
                } else if (vma->vm_start >= current_vm->vm_end) {
                        link = &current->right;
                } else {
                        return -1;
                }
        }
        
        rb_insert(&vm_tree->r_tree, &vma->rb_node, parent, link);
        return 0;
}




/* Does nothing if vma is NULL. NOTHING! NOTHIIINGNGGG!!! */
void vma_remove(vma_region_t *vm_tree, kernel_vma_t *_Nullable vma)
{
        if (!vma) return;

        rb_remove(&vm_tree->r_tree, &vma->rb_node);
        kslab_free(&vma_cache, vma);
}




kernel_vma_t *vma_search(vma_region_t *vm_tree, virt_addr_t addr)
{
        struct rb_node *node = vm_tree->r_tree.root;
        kernel_vma_t *v = NULL;

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




void vma_remove_by_addr(vma_region_t *vm_tree, virt_addr_t addr)
{
        // These are separate to help debugging.
        kernel_vma_t *v = vma_search(vm_tree, addr);
        vma_remove(vm_tree, v);
}



int vma_subsystem_init(void)
{
        int ret = kslab_cache_create(&vma_cache, VMA_IDENTIFIER, sizeof(kernel_vma_t), alignof(kernel_vma_t), 0);
        if (ret != 0) return 1;
        return 0;
}

