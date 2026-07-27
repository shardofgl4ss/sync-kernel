CC			:= x86_64-elf-gcc
LD			:= x86_64-elf-ld
AR			:= x86_64-elf-ar
AS			:= x86_64-elf-as
 
DEBUG_FLAGS		:= -ggdb3 -Wa,-g -Wl,-g -fno-omit-frame-pointer
REL_FLAGS		:= -O2 -Wl,-s

LINKER_BOOT_FILE	:= bootldr.ld
LINKER_KERNEL_FILE	:= kernel.ld
LINKER_FILE		:= linker.ld

CFLAGS_KERNELSPACE	:= -mno-red-zone -fno-pie -fno-pic
CFLAGS_STANDALONE	:= -ffreestanding -fno-builtin -fno-stack-protector -nostdinc -isystem $(shell $(CC) -print-file-name=include)
CFLAGS_COMMON		:= -Wall -fcf-protection=none
LDFLAGS			:= -no-pie -nostdlib -z noexecstack
LDFLAGS_BOOT 		:= $(LDFLAGS) -m elf_i386 --oformat binary
LDFLAGS_KRNL 		:= $(LDFLAGS) -m elf_x86_64
