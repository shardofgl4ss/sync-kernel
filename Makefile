include			config.mk

PROJ_DIR		:= $(CURDIR)
BUILD_DIR		:= build
SRC_DIR			:= src

BUILD_TYPE		?= debug

OBJ_DIR			:= $(BUILD_DIR)/obj
BIN_DIR			:= $(BUILD_DIR)/bin


UEFI_DIR		:= uefi
UEFI_CODE		:= $(UEFI_DIR)/OVMF_CODE.fd
UEFI_VARS		:= $(UEFI_DIR)/OVMF_VARS.fd


SRC_KERNEL_DIR		:= $(SRC_DIR)/kernel
SRC_KERNEL_DRIVERS_DIR	:= $(SRC_KERNEL_DIR)/drivers


DISK_IMAGE		:= $(BUILD_DIR)/disk.img
KERNEL_ELF		:= $(BIN_DIR)/kernel.elf
GRUB_FILE		:= $(BUILD_DIR)/BOOTX64.EFI


FIND_FLAGS_EXT		:= \( -name '*.S' -o -name '*.s' -o -name '*.c' \)
FIND_FLAGS_HDRS		:= \( -name '*.h' \)
SRCS			:= $(shell find $(SRC_DIR) -type f $(FIND_FLAGS_EXT))

SRCS_KERNEL             := $(filter $(SRC_KERNEL_DIR)/%,$(SRCS))

CFLAGS			:= $(CFLAGS_COMMON) $(CFLAGS_STANDALONE) $(CFLAGS_KERNELSPACE) -std=$(C_STD)

# include			$(SRC_DIR)/module.mk

OBJS_KERNEL		:= $(SRCS_KERNEL:%=$(OBJ_DIR)/%.o)


DEPS			:= $(OBJS_KERNEL:.o=.d) 

DEPFLAGS		= -MT $@ -MMD -MP -MF $(@:.o=.d)

ASFLAGS_KRNL 		:= -I$(SRC_DIR) -I$(SRC_DIR)/include -g

BUILD_DIRS		:= $(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)/tools
OBJ_SUBDIRS		:= $(sort $(dir $(OBJS_KERNEL)))


$(OBJS_KERNEL):		LOCAL_FLAGS := $(ASFLAGS_KRNL) -I$(SRC_DIR) -I$(SRC_DIR)/include -I$(SRC_KERNEL_DIR) -I$(SRC_KERNEL_DRIVERS_DIR) -m64


.PHONY: all image kernel clean standalone run

all: image

image: 		$(DISK_IMAGE)
kernel: 	$(KERNEL_ELF) $(KERNEL_ELF)
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
	QEMU_FLAGS += -s -S -d cpu_reset -debugcon stdio -global isa-debugcon.iobase=0xe9
endif



# KVM acceleration with host cpu make it impossible to debug with GDB.
# But it *is* closer to true hardware behavior then base QEMU.
run: $(DISK_IMAGE) $(UEFI_CODE) $(UEFI_VARS)
	qemu-system-x86_64 $(QEMU_FLAGS)


$(DISK_IMAGE): $(KERNEL_ELF) $(GRUB_FILE)
	dd if=/dev/zero of=$@ bs=1M count=64
	mkfs.fat -F 32 -n 'SYOS' $@
	mmd -i $@ "::/boot"
	mmd -i $@ "::/EFI"
	mmd -i $@ "::/EFI/BOOT"

	mcopy -i $@ $(KERNEL_ELF) "::/boot/kernel.elf"
	mcopy -i $@ $(GRUB_FILE) "::/EFI/BOOT/BOOTX64.EFI"


$(GRUB_FILE): grub.cfg
	@mkdir -p $(dir $@)
	grub-mkstandalone \
		-O x86_64-efi \
		-o $@ \
		--modules="fat part_gpt multiboot2" \
		'boot/grub/grub.cfg=grub.cfg' 


$(KERNEL_ELF): $(OBJS_KERNEL)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS_KRNL) -o $@ $^


$(OBJ_DIR)/%.S.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(LOCAL_FLAGS) -c -o $@ $<

$(OBJ_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(LOCAL_FLAGS) -c -o $@ $<

$(OBJ_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(LOCAL_FLAGS) -c -o $@ $<

-include $(DEPS)

clean:
	@if [ -d $(BUILD_DIR) ]; then \
		rm -r $(BUILD_DIR); \
	fi

