// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "vma.h"

#include "kstring.h"
#include "macros.h"
#include "pagetbl.h"
#include "panic.h"
#include "rbtree.h"
#include "slab.h"
#include "types.h"

extern char KERNEL_VMMMAP[];
static constexpr u64 KERNEL_VMMAP_LEN = (1024ULL * 1024ULL * 1024ULL * 1024ULL) * 64ULL;

const char *VMA_IDENTIFIER = "kernel_vma_struct";


vma_region_t vma_tree = {};
kslab_cache_t vma_cache = {};




static inline void vma_cache_free(void *p) { kslab_free(&vma_cache, p); }
static inline void *vma_cache_alloc(void) { return kslab_alloc(&vma_cache); }
static inline kernel_vma_t *vma_struct_from_rb_node(struct rb_node *x) { return substruct_owner(x, kernel_vma_t, rb_node); }




int vma_insert(vma_region_t *vm_tree, kernel_vma_t *vma)
{
        struct rb_node **link = &vm_tree->r_tree.root;
        struct rb_node *parent = NULL;

        while (*link != NULL) {
                struct rb_node *current = *link;
                kernel_vma_t *current_vm = vma_struct_from_rb_node(current);

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
        if (unlikely(!vma)) return;

        rb_remove(&vm_tree->r_tree, &vma->rb_node);
        kslab_free(&vma_cache, vma);
}




kernel_vma_t *vma_search(vma_region_t *vm_tree, virt_addr_t addr)
{
        struct rb_node *node = vm_tree->r_tree.root;
        kernel_vma_t *v = NULL;

        while (node) {
                v = vma_struct_from_rb_node(node);

                if (addr >= v->vm_start && addr < v->vm_end) goto done;
                if (addr < v->vm_start) node = node->left;
                if (addr >= v->vm_end) node = node->right;
        }

        return VMA_INVALID_PTR;
done:
        return v;
}




void vma_remove_by_addr(vma_region_t *vm_tree, virt_addr_t addr)
{
        // These are separate to help debugging.
        kernel_vma_t *v = vma_search(vm_tree, addr);
        vma_remove(vm_tree, v);
}




_hot_
void vma_free_area(vma_region_t *vm_tree, virt_addr_t addr)
{
        kernel_vma_t *vma = vma_search(vm_tree, addr);
        if (unlikely(vma == VMA_INVALID_PTR)) return;
        rb_remove(&vm_tree->r_tree, &vma->rb_node);
        vma_cache_free(vma);
}




static int vma_assign_struct(kernel_vma_t **vma, virt_addr_t begin, const usize sz, vma_flags_t flags)
{
        *vma = vma_cache_alloc();

        if (unlikely(*vma == VMA_INVALID_PTR)) return 1;

        kernel_vma_t *v = *vma;
        memset(v, 0, sizeof(*v));

        v->vm_start = begin;
        v->vm_end = begin + sz;
        v->vm_flags = flags;

        return 0;
}




_hot_ _malloc_arg_(vma_free_area, 1)
kernel_vma_t *vma_alloc_area(vma_region_t *vm_tree, usize area_size, const vma_flags_t flags, void *_Nullable extra)
{
        (void)extra;

        area_size = PAGE_ALIGNUP(area_size);
        struct rb_node *root = vm_tree->r_tree.root;
        usize region_depth = vm_tree->r_end - vm_tree->r_start;

        if (unlikely(area_size > region_depth)) return VMA_INVALID_PTR;

        kernel_vma_t *vma = VMA_INVALID_PTR;
        int ret = 1;

        if (unlikely(root == NULL)) {
                ret = vma_assign_struct(&vma, vm_tree->r_start, area_size, flags);
                goto done;
        }

        struct rb_node *rb_lowest = rb_first(&vm_tree->r_tree);
        struct rb_node *rb_highest = rb_last(&vm_tree->r_tree);

        if (unlikely(rb_lowest == NULL || rb_highest == NULL)) {
                return VMA_INVALID_PTR;
        }

        kernel_vma_t *lowest = vma_struct_from_rb_node(rb_lowest);
        kernel_vma_t *highest = vma_struct_from_rb_node(rb_highest);

        if (lowest->vm_start - vm_tree->r_start >= area_size) {
                ret = vma_assign_struct(&vma, vm_tree->r_start, area_size, flags);
                goto done;
        } else if (vm_tree->r_end - highest->vm_end >= area_size) {
                ret = vma_assign_struct(&vma, highest->vm_end, area_size, flags);
                goto done;
        }

        kernel_vma_t *node = lowest;
        virt_addr_t prev_end = node->vm_end;

        while (node != highest) {
                prev_end = node->vm_end;
                node = vma_struct_from_rb_node(rb_next(&node->rb_node));

                const usize vm_gap = node->vm_start - prev_end;

                if (vm_gap >= area_size) {
                        ret = vma_assign_struct(&vma, prev_end, area_size, flags);
                        goto done;
                }
        }

done:
        if (unlikely(ret != 0)) return VMA_INVALID_PTR;
        vma_insert(vm_tree, vma);
        return vma;
}




int vma_subsystem_init(void)
{
        int ret = kslab_cache_create(&vma_cache, VMA_IDENTIFIER, sizeof(kernel_vma_t), alignof(kernel_vma_t), 0);

        if (unlikely(ret != 0)) return 1;

        vma_tree.r_start = (virt_addr_t)get_kernel_vmmap();
        vma_tree.r_end = (virt_addr_t)KERNEL_VMMMAP + KERNEL_VMMAP_LEN;

        return 0;
}

