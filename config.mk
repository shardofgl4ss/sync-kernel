CC			:= x86_64-elf-gcc
LD			:= x86_64-elf-ld
AR			:= x86_64-elf-ar
AS			:= x86_64-elf-as
OBJCOPY			:= x86_64-elf-objcopy
 
DEBUG_FLAGS		:= -ggdb3 -Wa,-g -Wl,-g -fno-omit-frame-pointer
REL_FLAGS		:= -O2 -Wl,-s

CFLAGS_KERNELSPACE	:= -mno-red-zone -fno-pie -fno-pic
CFLAGS_STANDALONE	:= -ffreestanding -fno-builtin -fno-stack-protector -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CFLAGS_COMMON		:= -Wall -fcf-protection=none
LDFLAGS			:= -no-pie -nostdlib -z noexecstack
LDFLAGS_BOOT_S1		:= $(LDFLAGS) -T stage1.ld -m elf_i386 --oformat binary
LDFLAGS_BOOT_S2		:= $(LDFLAGS) -T stage2.ld -m elf_x86_64
LDFLAGS_KRNL 		:= $(LDFLAGS) -T kernel.ld -m elf_x86_64
