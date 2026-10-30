# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Sync Shard

DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))


INC_DIRS +=

C_SOURCES += $(DIR)/buddy.c \
	     $(DIR)/kmalloc.c \
	     $(DIR)/slab.c \
	     $(DIR)/vma.c \
	     $(DIR)/vmalloc.c

ASM_SOURCES +=

