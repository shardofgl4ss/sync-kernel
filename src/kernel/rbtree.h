// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once


enum rb_colors : bool {
        RB_RED = true,
        RB_BLACK = false,
};

struct rb_node {
        struct rb_node *left;
        struct rb_node *right;
        struct rb_node *parent;

        enum rb_colors color;
};


struct rb_tree {
        struct rb_node *root;
};


/* requires a subsystem to properly identify where to insert the node, as the RB tree does not have the data information. */
extern void rb_insert(struct rb_tree *tree, struct rb_node *node, struct rb_node *parent, struct rb_node **link);
/* returns a rb_node to be freed by any subsystem. */
extern struct rb_node *rb_remove(struct rb_tree *tree, struct rb_node *node);
extern struct rb_tree rb_inittree(void);

