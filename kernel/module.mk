# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Sync Shard

DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))


INC_DIRS += $(DIR) $(DIR)/drivers $(DIR)/mm $(DIR)/include

C_SOURCES += $(DIR)/bootparams.c \
	     $(DIR)/boot.c \
	     $(DIR)/cpu_feats.c \
	     $(DIR)/gdt.c \
	     $(DIR)/idt.c \
	     $(DIR)/irq.c \
	     $(DIR)/isr.c \
	     $(DIR)/kstring.c \
	     $(DIR)/main.c \
	     $(DIR)/pagetbl.c \
	     $(DIR)/panic.c \
	     $(DIR)/rbtree.c \
	     $(DIR)/stdio.c

ASM_SOURCES += $(DIR)/entry.S \
	       $(DIR)/gdt.S \
	       $(DIR)/splash.S \
	       $(DIR)/isr.S

