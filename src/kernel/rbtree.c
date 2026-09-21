// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "types.h"
#include "macros.h"



enum rb_colors : bool {
        RB_RED = true,
        RB_BLACK = false,
};


struct rb_node {
        struct rb_node *left;
        struct rb_node *right;
        struct rb_node *parent;

        enum rb_colors color;

        u64 data;
};


struct rb_tree {
        struct rb_node *root;
};

typedef struct rb_node rbnode;
typedef struct rb_tree rbtree;


extern void rb_lrotate(struct rb_tree *tree, struct rb_node *node);
extern void rb_rrotate(struct rb_tree *tree, struct rb_node *node);

extern void rb_fixins(struct rb_tree *tree, struct rb_node *node);
extern void rb_insert(struct rb_tree *tree, u64 data);

extern struct rb_node *rb_getroot(struct rb_node *node);
extern struct rb_tree *rb_inittree();



struct rb_node *rb_getroot(struct rb_node *node)
{
        rbnode *root = node;

        while (root->parent != NULL) {
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


void rb_insert(struct rb_tree *tree, u64 data)
{

}

