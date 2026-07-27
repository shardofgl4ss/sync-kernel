#define BOOT_ORG        _begin

.code16
.section .text
        .global __start

__start:
        jmp.s    start
        nop

fdb_oem:                        .ascii "MSWIN4.1" # 8 bytes only
fdb_bytes_per_sector:           .word 512
fdb_sectors_per_cluster:        .byte 1
fdb_reserved_sectors:           .word 1
fdb_fat_count:                  .byte 2
fdb_dentries:                   .word 0xe0
fdb_total_sectors:              .word 2880
fdb_media_descriptor_table:     .byte 0xf0
fdb_sectors_per_fat:            .word 9
fdb_sectors_per_track:          .word 18
fdb_heads:                      .word 2
fdb_hidden:                     .long 0
fdb_large:                      .long 0

ebr_drive_number:               .byte 0 # 0 = floppy, 0x80 = hdd but dont change it
                                .byte 0
ebr_signature:                  .byte 0x29
ebr_volume_id:                  .byte 0x12, 0x34, 0x56, 0x78
ebr_volume_label:               .ascii "SYN_FLOPPY " # 11 bytes only
ebr_system_id:                  .ascii "FAT12   "   # 8 bytes only

#define BIOS_VIDEO_PRINT        0x0e
#define BIOS_DISKIO_DPARAMS     0x08

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


#define KCLUSTER_OFFSET          31

#define KERNEL_LOAD_SEGMENT    0x1000
#define KERNEL_LOAD_OFFSET     0

start:
        # segment setup
        xorw    %ax, %ax

        movw    %ax, %ds
        movw    %ax, %es

        # stack setup
        movw    %ax, %ss
        movw    $_begin, %sp

        ljmp    $0x0000, $.Lafter
.Lafter:
        movb    %dl, (ebr_drive_number)

        leaw    msg_boot_loading, %si
        call    puts

        # time to figure out disk information
        # just in case the structure above
        # was corrupted. just in case.
        push    %es
        mov     $BIOS_DISKIO_DPARAMS, %ah
        stc
        int     $BIOS_DISKIO
        jc      floppy_err

        pop     %es
        andb    $0x3f, %cl
        xor     %ch, %ch
        movw    %cx, (fdb_sectors_per_track)

        inc     %dh
        movb    %dh, (fdb_heads)

        # getting LBA of root (reserved + fat count * sectors per fat)
        movw    (fdb_sectors_per_fat), %ax
        movb    (fdb_fat_count), %bl
        xor     %bh, %bh
        mul     %bx

        addw    (fdb_reserved_sectors), %ax # LBA of root acquired
        push    %ax

        # getting size of root dentry (32 * dentries) / bytes per sector
        movw    (fdb_sectors_per_fat), %ax
        shl     $5, %ax
        xor     %dx, %dx
        divw    (fdb_bytes_per_sector)

        test    %dx, %dx
        jz      .Lafter_root
        inc     %ax # if div remainder != 0, sectors are partially full
.Lafter_root:
        
        # tiem tew wead woot uwu
        mov     %al, %cl        # size of root in cl
        pop     %ax             # LBA of root dentry in ax
        
        movb    (ebr_drive_number), %dl
        leaw    (buffer), %bx   # es:bx, buffer
        call    read_disk

        push    %cx
        xor     %bx, %bx
        leaw    (buffer), %di
.Lfind_kernel: 
        lea     kernel_name, %si
        mov     $kernel_name_len, %cx
        push    %di
        repe    cmpsb
        pop     %di
        je      .Lfound_kernel

        add     $0x20, %di      # 32 is the size of a fat12 dentry
        inc     %bx

        cmpw    (fdb_dentries), %bx
        jl      .Lfind_kernel

        jmp     kernel_not_found_err

.Lfound_kernel:
        # %di should still have the dentry addr
        movw    0x1a(%di), %ax
        movw    %ax, (kcluster)

        # popcorn kernel read tiem
        movw    (fdb_reserved_sectors), %ax
        leaw    (buffer), %bx
        movb    (fdb_sectors_per_fat), %cl
        movb    (ebr_drive_number), %dl
        call    read_disk

        # FAT chain processing.
        movw    $KERNEL_LOAD_SEGMENT, %bx
        movw    %bx, %es
        mov     $KERNEL_LOAD_OFFSET, %bx

.Lload_kernel:

        # start_sector = reserved + fats + root dentry size
        # first cluster is (kcluster - 2) * sectors per cluster + start_sector
        movw    (kcluster), %ax
        addw    $KCLUSTER_OFFSET, %ax

        movb    $1, %cl
        movb    (ebr_drive_number), %dl
        call    read_disk

        addw    (fdb_bytes_per_sector), %bx
        jnc     1f

        push    %ax
        movw    %es, %ax
        addw    $0x1000, %ax
        movw    %ax, %es
        pop     %ax
1:

        movw    (kcluster), %ax
        movw    $3, %cx
        mul     %cx
        mov     $2, %cx
        div     %cx     # %ax = dentry, %dx = cluster % 2

        leaw    (buffer), %si
        addw    %ax, %si
        movw    %ds:(%si), %ax # dentry at index %ax

        or      %dx, %dx
        jz      .Leven

.Lodd: 
        shr     $4, %ax
        jmp     .Lnext_cluster
.Leven:
        andw    $0x0FFF, %ax
.Lnext_cluster:
        
        cmpw    $0x0FF8, %ax
        jge     .Lkernel_read_fini

        movw    %ax, (kcluster)
        jmp     .Lload_kernel

.Lkernel_read_fini:
        movb    (ebr_drive_number), %dl

        cli
        movw    $KERNEL_LOAD_SEGMENT, %ax
        mov     %ax, %ds
        mov     %ax, %es

        ljmp     $KERNEL_LOAD_SEGMENT, $KERNEL_LOAD_OFFSET

halt:
        cli
        hlt
        jmp     halt

kernel_not_found_err:
        mov     emsg_kernel_not_found, %si
        call    puts
        jmp     wait_key_reboot

floppy_err:
        leaw    emsg_disk_read, %si
        call    puts

wait_key_reboot:
        xorb    %ah, %ah
        int     $BIOS_KEYBOARDIO
        jmp     0xFFFF



# prints string to screen
# parameters:
#       - %ds:%si string
# returns:
#       - none
puts:
        push    %si
        push    %ax
.Lputs_loop:
        lodsb
        orb     %al, %al
        jz      .Lputs_done

        movb    $BIOS_VIDEO_PRINT, %ah
        xorb    %bh, %bh
        int     $BIOS_VIDEO

        jmp     .Lputs_loop
.Lputs_done:
        pop     %ax
        pop     %si
        ret


# Converts Logical Block Address to CHS
# params:
#       - %ax: LBA addr
# returns:
#       - sector number: %cx (bits 0-5)
#       - cylinder number: %cx (bits 6-15)
#       - head: %dh
lba_to_chs:
        push    %ax
        push    %dx

        xor     %dx, %dx
        divw    (fdb_sectors_per_track)

        inc     %dx # iirc, sectors count from 1, so increment by 1.
        mov     %dx, %cx

        xor     %dx, %dx
        divw    (fdb_heads)

        mov     %dl, %dh
        mov     %al, %ch

        shl     $6, %ah
        or      %ah, %cl

        pop     %ax
        mov     %al, %dl
        pop     %ax
        ret




# Reads sectors from disk.
# parameters:
#       - %ax: LBA
#       - %cl: sectors to read (128 is max)
#       - %dl: drive number
#       - %es:%bx: output buffer
# returns:
#       - none
read_disk:
        push    %ax
        push    %bx
        push    %cx
        push    %dx
        push    %di

        push    %cx
        call    lba_to_chs
        pop     %ax

        mov     $0x02, %ah
        mov     $3, %di
.Lread_disk.retry:
        pusha

        stc
        int     $BIOS_DISKIO
        jnc     .Lread_disk.done

        popa

        call    reset_disk
        dec     %di
        test    %di, %di
        jnz     .Lread_disk.retry
        jmp     floppy_err
.Lread_disk.done:
        popa

        pop     %di
        pop     %dx
        pop     %cx
        pop     %bx
        pop     %ax

        ret


# reset disk controller
# parameters:
#       - %dl: drive number to reset
# returns:
#       - none
reset_disk:
        pusha
        xor     %ah, %ah
        stc
        int     $BIOS_DISKIO
        jc      floppy_err
        popa
        ret




msg_boot_loading:       .asciz "Loading..."
emsg_kernel_not_found:  .asciz "NO KERNEL"
emsg_disk_read:         .asciz "DISK ERR"
kernel_name:            .ascii "KERNEL  BIN"
.set kernel_name_len, . - kernel_name

.if kernel_name_len != 11
        .err
.endif

kcluster:         .word 0

.fill 510 - (. - __start), 1, 0
.word 0xaa55

buffer:

