# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Sync Shard

include			config.mk

PROJ_DIR		:= $(CURDIR)
SRC_DIR			:= kernel
IMG_DIR			:= img

UEFI_DIR		:= uefi
UEFI_CODE		:= $(UEFI_DIR)/OVMF_CODE.fd
UEFI_VARS		:= $(UEFI_DIR)/OVMF_VARS.fd


BUILD_TYPE		?= debug


DISK_IMAGE		:= $(IMG_DIR)/disk.img
KERNEL_ELF		:= $(IMG_DIR)/kernel.elf
GRUB_FILE		:= $(IMG_DIR)/BOOTX64.EFI


INC_DIRS		:=
C_SOURCES		:=
ASM_SOURCES		:=


MODULE_FILES		:= $(sort $(shell find $(SRC_DIR) -type f -name 'module.mk' ))
include 		$(MODULE_FILES)


SOURCES			:= $(C_SOURCES) $(ASM_SOURCES)
OBJECTS			:= $(addsuffix .o, $(SOURCES))
DEPS			:= $(OBJECTS:.o=.d) 


CFLAGS			:= $(CFLAGS_COMMON) $(CFLAGS_STANDALONE) $(CFLAGS_KERNELSPACE) -std=$(C_STD)
CPPFLAGS		+= $(addprefix -I,$(INC_DIRS))
ASMFLAGS 		:= -I$(SRC_DIR) -I$(SRC_DIR)/include -g
DEPFLAGS		= -MT $@ -MMD -MP -MF $(@:.o=.d)


.PHONY: all image kernel clean distclean standalone run

all: image

image: 		$(DISK_IMAGE) $(DISK_IMAGE).kernel $(DISK_IMAGE).grub
kernel: 	$(KERNEL_ELF)
standalone:	$(GRUB_FILE)

QEMU_UEFI		:= -drive if=pflash,format=raw,readonly=on,file=$(UEFI_CODE) \
			   -drive if=pflash,format=raw,file=$(UEFI_VARS)

QEMU_DISK 		:= -drive format=raw,file=$(DISK_IMAGE)

QEMU_FLAGS		:= $(QEMU_UEFI) $(QEMU_DISK) -no-reboot -no-shutdown

QEMU_KVM		?= 0

ifeq ($(QEMU_KVM), 1)
	QEMU_FLAGS += -cpu host -accel kvm
endif


ifeq ($(BUILD_TYPE), release)
	CFLAGS += $(REL_FLAGS)
else
	CFLAGS += $(DEBUG_FLAGS)
	CPPFLAGS += -DDEBUG
	QEMU_FLAGS += -s -S -d cpu_reset -debugcon stdio -global isa-debugcon.iobase=0xe9
endif



# KVM acceleration with host cpu make it impossible to debug with GDB.
# But it *is* closer to true hardware behavior then base QEMU.
run: image $(UEFI_CODE) $(UEFI_VARS)
	@qemu-system-x86_64 $(QEMU_FLAGS)


$(KERNEL_ELF): $(OBJECTS)
	@mkdir -p $(dir $@)
	@printf "  LD        %s\n" "$@"
	@$(LD) $(LDFLAGS_KRNL) -o $@ $^


$(DISK_IMAGE).kernel: $(KERNEL_ELF) | $(DISK_IMAGE)
	@mkdir -p $(dir $@)
	@printf "  MCOPY     %s\t%s\n" "$(DISK_IMAGE)" "$(KERNEL_ELF)"
	@mcopy -o -i $(DISK_IMAGE) $(KERNEL_ELF) "::/boot/kernel.elf"
	@touch $@
$(DISK_IMAGE).grub: $(GRUB_FILE) | $(DISK_IMAGE)
	@mkdir -p $(dir $@)
	@printf "  MCOPY     %s\t%s\n" "$(DISK_IMAGE)" "$(GRUB_FILE)"
	@mcopy -o -i $(DISK_IMAGE) $(GRUB_FILE) "::/EFI/BOOT/BOOTX64.EFI"
	@touch $@



$(DISK_IMAGE):
	@mkdir -p $(dir $@)
	@printf "  TR        %s\n" "$@"
	@truncate -s 64M $@
	@printf "  MKFS      %s\t%s\n" "$@" "FAT32"
	@mkfs.fat -F 32 -n 'SYOS' $@ >/dev/null 2>&1
	@mmd -i $@ "::/boot"
	@mmd -i $@ "::/EFI"
	@mmd -i $@ "::/EFI/BOOT"
	@rm -f $(DISK_IMAGE).kernel $(DISK_IMAGE).grub


$(GRUB_FILE): grub.cfg
	@mkdir -p $(dir $@)
	@printf "  BOOTLDR   %s\n" "$@"
	@grub-mkstandalone \
		-O x86_64-efi \
		-o $@ \
		--modules="fat part_gpt multiboot2" \
		'boot/grub/grub.cfg=grub.cfg' 


%.S.o: %.S
	@printf "  AS        %s\n" "$@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $(LOCAL_FLAGS) -c -o $@ $<

%.s.o: %.s
	@printf "  AS        %s\n" "$@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $(LOCAL_FLAGS) -c -o $@ $<

%.c.o: %.c
	@printf "  CC        %s\n" "$@"
	@$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $(LOCAL_FLAGS) -c -o $@ $<


-include $(DEPS)

clean:
	@rm -f $(OBJECTS)
	@rm -f $(DEPS)
	@rm -f $(KERNEL_ELF)
	@printf "cleaned build artifacts\n"


distclean: clean
	@rm -f $(DISK_IMAGE)
	@rm -f $(DISK_IMAGE).kernel $(DISK_IMAGE).grub
	@rm -f $(GRUB_FILE)
	@printf "cleaned boot and disk files\n"

