.code16
.section .text
        .global start


#define KSEGMENT                0x2000
#define BIOS_VIDEO_PRINT        0x0e
#define BIOS_VIDEO              0x10
# Prints character to screen in TTY mode.
# %ah reg must be $0x0e, 
# %al to the ascii character, 
# %bh to the page number
# %bl to foreground pix color (graphics mode only)

#define BIOS_EQUIPCHK           0x11
#define BIOS_MEMSZ              0x12
#define BIOS_DISKIO             0x13
#define BIOS_SERIALCOMM         0x14
#define BIOS_CASETTE            0x15
#define BIOS_KEYBOARDIO         0x16




start:
        cli

        lgdt    gdt_descriptor
        movl    %cr0, %eax
        orl     $1, %eax
        movl    %eax, %cr0

        ljmp    $0x08, $prot_mode_start
.code32
prot_mode_start:
        movw    $0x10, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        movw    %ax, %ss

halt:
        cli
        hlt


.align 4
gdt_start:
        .quad 0x0000000000000000    # null
        .quad 0x00cf9a000000ffff    # code x08
        .quad 0x00cf92000000ffff    # data 0x10
gdt_end:

gdt_descriptor:
        .word gdt_end - gdt_start - 1
        .long gdt_start



