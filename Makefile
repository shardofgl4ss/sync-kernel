include			config.mk

PROJ_DIR		:= $(CURDIR)
BUILD_DIR		:= build
SRC_DIR			:= src
TOOLS_DIR		:= tools

OBJ_DIR			:= $(BUILD_DIR)/obj
BIN_DIR			:= $(BUILD_DIR)/bin


SRC_KERNEL_DIR		:= $(SRC_DIR)/kernel
SRC_KERNEL_INCDIR	:= $(SRC_KENREL_DIR)/include
SRC_KERNEL_DRIVERS_DIR	:= $(SRC_KERNEL_DIR)/drivers
SRC_BOOT_DIR		:= $(SRC_DIR)/boot
SRC_BOOT_S2_DIR		:= $(SRC_BOOT_DIR)/stage2
SRC_BOOT_S1_DIR		:= $(SRC_BOOT_DIR)/stage1


# At the moment, only stage 1 is used. This will probably change
# once the kernel grows bigger to set up GDT better, rudimentary drivers,
# etc before calling kernel_main. The file load name str in stage1.S will also need to be changed.

FLOPPY_IMG		:= $(BUILD_DIR)/floppy.img
KERNEL_BIN		:= $(BIN_DIR)/kernel.bin
BOOT_S2_BIN		:= $(BIN_DIR)/boot_s2.bin
BOOT_S1_BIN		:= $(BIN_DIR)/boot_s1.bin

KERNEL_ELF		:= $(BIN_DIR)/kernel.elf

FIND_FLAGS_EXT		:= \( -name '*.S' -o -name '*.s' -o -name '*.c' \)
SRCS			:= $(shell find $(SRC_DIR) -type f $(FIND_FLAGS_EXT))

SRCS_KERNEL		:= $(filter $(SRC_KERNEL_DIR)/%,$(SRCS))
SRCS_BOOT_S2 		:= $(filter $(SRC_BOOT_S2_DIR)/%,$(SRCS))
SRCS_BOOT_S1 		:= $(filter $(SRC_BOOT_S1_DIR)/%,$(SRCS))

##### CFLAGS
CFLAGS			:= $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $(CFLAGS_STANDALONE) $(CFLAGS_KERNELSPACE) -std=c2x

# include			$(SRC_DIR)/module.mk

OBJS_KERNEL		:= $(SRCS_KERNEL:%=$(OBJ_DIR)/%.o)
OBJS_BOOT_S2		:= $(SRCS_BOOT_S2:%=$(OBJ_DIR)/%.o)
OBJS_BOOT_S1		:= $(SRCS_BOOT_S1:%=$(OBJ_DIR)/%.o)


DEPS			:= $(OBJS_BOOT_S1:.o=.d) $(OBJS_BOOT_S2:.o=.d) $(OBJS_KERNEL:.o=.d) 

DEPFLAGS		= -MT $@ -MMD -MP -MF $(@:.o=.d)

ASFLAGS_BOOT_S1         := -I$(SRC_DIR) -I$(SRC_DIR)/include -m16 -g
ASFLAGS_KRNL 		:= -I$(SRC_DIR) -I$(SRC_DIR)/include -g

BUILD_DIRS		:= $(BIN_DIR) $(OBJ_DIR) $(BIN_DIR)/tools
OBJ_SUBDIRS		:= $(sort $(dir $(OBJS_BOOT_S1) $(OBJS_KERNEL)))


$(OBJS_BOOT_S1):	LOCAL_FLAGS := $(ASFLAGS_BOOT_S1) -I$(SRC_BOOT_DIR) -I$(SRC_BOOT_S1_DIR)
$(OBJS_BOOT_S2):	LOCAL_FLAGS := $(ASFLAGS_BOOT_S1) -I$(SRC_BOOT_DIR) -I$(SRC_BOOT_S2_DIR)
$(OBJS_KERNEL):		LOCAL_FLAGS := $(ASFLAGS_KRNL) -I$(SRC_KERNEL_DIR) -I$(SRC_KERNEL_DRIVERS_DIR) -I$(SRC_KERNEL_INCDIR) -m64


.PHONY: all floppy kernel bootloader clean

all: floppy

floppy: 	$(FLOPPY_IMG)
bootloader: 	$(BOOT_S1_BIN) $(BOOT_S2_BIN)
kernel: 	$(KERNEL_ELF) $(KERNEL_BIN)


$(FLOPPY_IMG): $(BOOT_S1_BIN) $(BOOT_S2_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=512 count=2880
	mkfs.fat -F 12 -n 'SYOS' $@
	dd if=$(BOOT_S1_BIN) of=$@ conv=notrunc
	mcopy -i $@ $(BOOT_S2_BIN) "::stage2.bin"
	mcopy -i $@ $(KERNEL_BIN) "::kernel.bin"


$(KERNEL_BIN): $(KERNEL_ELF)
	@mkdir -p $(dir $@)
	objcopy -O binary $^ $@



$(KERNEL_ELF): $(OBJS_KERNEL)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS_KRNL) -T$(LINKER_KERNEL_FILE) -o $@ $^

$(BOOT_S2_BIN): $(OBJS_BOOT_S2)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS_BOOT) -T$(LINKER_FILE) -o $@ $^


$(BOOT_S1_BIN): $(OBJS_BOOT_S1)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS_BOOT) -T$(LINKER_BOOT_FILE) -o $@ $^
	@size=$$(stat -c %s $@); \
	if [ $$size -ne 512 ]; then \
		echo "bootloader is not 512B, got $$size"; \
		exit 1; \
	fi


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
	else \
		echo "Nothing to clean."; \
	fi

