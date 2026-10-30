# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Sync Shard

DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))


INC_DIRS +=

C_SOURCES += $(DIR)/console.c \
	     $(DIR)/serial.c

ASM_SOURCES +=

