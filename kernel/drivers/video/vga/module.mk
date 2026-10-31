# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Sync Shard

DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))


INC_DIRS +=

C_SOURCES += $(DIR)/vga.c \
	     $(DIR)/vga_graphics.c \
	     $(DIR)/vga_text.c

ASM_SOURCES +=

