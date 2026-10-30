# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Sync Shard

DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))


INC_DIRS += $(DIR)/serial $(DIR)/video

C_SOURCES +=

ASM_SOURCES +=

