TOOLCHAIN		?= x86_64-elf

CC			?= $(TOOLCHAIN)-gcc
LD			?= $(TOOLCHAIN)-ld
AR			?= $(TOOLCHAIN)-ar
AS			?= $(TOOLCHAIN)-as
OBJCOPY			?= $(TOOLCHAIN)-objcopy
 
DEBUG_FLAGS		:= -Og -ggdb3 -Wa,-g -Wl,-g
REL_FLAGS		:= -O2 -Wl,-s

C_STD			:= c2x
CFLAGS_KERNELSPACE	:= -mno-red-zone -fno-plt -fno-omit-frame-pointer -fno-strict-aliasing
CFLAGS_STANDALONE	:= -ffreestanding -fno-builtin -fno-stack-protector -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CFLAGS_COMMON		:= -Wall -fcf-protection=none
LDFLAGS			:= -no-pie -nostdlib -z noexecstack
LDFLAGS_KRNL 		:= $(LDFLAGS) -T kernel.ld -m elf_x86_64
