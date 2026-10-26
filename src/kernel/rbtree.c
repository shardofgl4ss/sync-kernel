// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "rbtree.h"

#include "types.h"
#include "macros.h"


typedef struct rb_node rbnode;
typedef struct rb_tree rbtree;


extern void rb_lrotate(struct rb_tree *tree, struct rb_node *node);
extern void rb_rrotate(struct rb_tree *tree, struct rb_node *node);

extern void rb_fixins(struct rb_tree *tree, struct rb_node *node);

extern struct rb_node *rb_getroot(struct rb_node *node);



static inline bool rb_chk_black(struct rb_node *x)
{
        return !x || x->color == RB_BLACK;
}
static inline bool rb_chk_red(struct rb_node *x)
{
        return x && x->color == RB_RED;
}

struct rb_node *rb_getroot(struct rb_node *node)
{
        rbnode *root = node;

        while (root && root->parent != NULL) {
                root = root->parent;
        }

        return root;
}


void rb_lrotate(struct rb_tree *tree, struct rb_node *x)
{
        rbnode *y = x->right;
        x->right = y->left;

        if (y->left != NULL) {
                y->left->parent = x;
        }

        y->parent = x->parent;

        if (x->parent == NULL) {
                tree->root = y;
        } else if (x == x->parent->left) {
                x->parent->left = y;
        } else {
                x->parent->right = y;
        }

        y->left = x;
        x->parent = y;
}


void rb_rrotate(struct rb_tree *tree, struct rb_node *x)
{
        rbnode *y = x->left;
        x->left = y->right;

        if (y->right != NULL) {
                y->right->parent = x;
        }

        y->parent = x->parent;

        if (x->parent == NULL) {
                tree->root = y;
        } else if (x == x->parent->right) {
                x->parent->right = y;
        } else {
                x->parent->left = y;
        }

        y->right = x;
        x->parent = y;
}


void rb_fixins(struct rb_tree *tree, struct rb_node *x)
{
        while (x != tree->root && x->parent->color == RB_RED) {
                rbnode *parent = x->parent;
                rbnode *grandparent = x->parent->parent;

                const bool unc_right = (parent == grandparent->left);

                rbnode *big_unc = (unc_right)
                        ? grandparent->right
                        : grandparent->left;

                if (big_unc != NULL && big_unc->color == RB_RED) {
                        parent->color = RB_BLACK;
                        big_unc->color = RB_BLACK;

                        grandparent->color = RB_RED;
                        x = grandparent;

                        continue;
                }

                rbnode *inner = (unc_right) ? parent->right : parent->left;

                if (x == inner) {
                        x = parent;

                        if (unc_right) {
                                rb_lrotate(tree, x);
                        } else {
                                rb_rrotate(tree, x);
                        }

                        parent = x->parent;
                        grandparent = parent->parent;
                }

		parent->color = RB_BLACK;
                grandparent->color = RB_RED;

                if (unc_right) {
                        rb_rrotate(tree, grandparent);
                } else {
                        rb_lrotate(tree, grandparent);
                }
        }

        tree->root->color = RB_BLACK;
}


void rb_fixdel(struct rb_tree *tree, struct rb_node *x)
{
        while (x != tree->root && x->color == RB_BLACK) {
                bool lrotate = (x->parent->left == x);

                struct rb_node *x_sibling = lrotate ? x->parent->right : x->parent->left;
                struct rb_node *sibling_inner = lrotate ? x_sibling->right : x_sibling->left;
                struct rb_node *sibling_outer = lrotate ? x_sibling->left : x_sibling->right;


                if (x_sibling && x_sibling->color == RB_RED) {
                        x_sibling->color = RB_BLACK;
                        x->parent->color = RB_RED;


                        if (lrotate) rb_lrotate(tree, x->parent);
                        else rb_rrotate(tree, x->parent);

                        x_sibling = (lrotate) ? x->parent->right : x->parent->left;
                        sibling_inner = lrotate ? x->parent->right : x->parent->left;
                        sibling_outer = lrotate ? x->parent->left : x->parent->right;

                        // x_sibling->color = RB_BLACK;
                }

                if (rb_chk_black(sibling_inner) && rb_chk_black(sibling_outer)) {
                        x_sibling->color = RB_RED;
                        x = x->parent;
                } else {
                        if (rb_chk_black(sibling_outer)) {
                                if (lrotate) {
                                        rb_rrotate(tree, x_sibling);
                                        x_sibling = x->parent->right;
                                        sibling_outer = x_sibling->left;
                                } else {
                                        rb_lrotate(tree, x_sibling);
                                        x_sibling = x->parent->left;
                                        sibling_outer = x_sibling->right;
                                }
                        }

                        x_sibling->color = x->parent->color;
                        x->parent->color = RB_BLACK;
                        if (sibling_outer) sibling_outer->color = RB_BLACK;

                        if (lrotate) rb_lrotate(tree, x->parent);
                        else rb_rrotate(tree, x->parent);

                        x = tree->root;
                }
        }

        if (x) x->color = RB_BLACK;
}


void rb_insert(struct rb_tree *tree, struct rb_node *node, struct rb_node *parent, struct rb_node **link)
{
        if (!node) return;

        node->left = NULL;
        node->right = NULL;
        node->parent = parent;
        node->color = RB_RED;

        *link = node;

        rb_fixins(tree, node);
}

struct rb_node *rb_remove(struct rb_tree *tree, struct rb_node *node)
{
        if (!node) return NULL;

        struct rb_node *x = {};
        struct rb_node **parent_ptr_node;

        if (node == tree->root) {
                parent_ptr_node = &tree->root;
        } else {
                parent_ptr_node = (node == node->parent->left) 
                        ? &node->parent->left 
                        : &node->parent->right;
        }

        if (!node->left && !node->right) {
                // zero children.

                *parent_ptr_node = NULL;

                if (node->color == RB_BLACK) {
                        rb_fixdel(tree, node);
                }


        } else if ((node->left && !node->right) || (!node->left && node->right)) {
                // one child.
                x = (node->left) ? node->left : node->right;
                *parent_ptr_node = x;
                x->parent = node->parent;
                x->color = RB_BLACK;
        } else {
                // two children.
                x = node->right;

                while (x->left) { 
                        x = x->left;
                }

                rb_remove(tree, x);

                *parent_ptr_node = x;

                x->left = node->left;
                if (x->left) x->left->parent = x;

                x->right = node->right;
                if (x->right) x->right->parent = x;


                x->color = node->color;
                x->parent = node->parent;
        }
        return node;
}


struct rb_tree rb_inittree(void)
{
        struct rb_tree t = {};
        return t;
}


