#define BOOT_ORG        _begin

.code16
.section .text
        .global __start
fat_jmp:
        jmp.s __start
        nop


# bpb:    .space 59 # FAT12 size
bpb:    .space 87 # FAT32 size




# Prints character to screen in TTY mode.
# %ah reg must be $0x0e
# %al to the ascii character
.set BIOS_INT_video,    0x10
.set BIOS_INT_check,    0x11
.set BIOS_INT_memsize,  0x12
.set BIOS_INT_disk,     0x13

### These are offsets. BS = Boot Sector, BPB = Bios Parameter Block

 # Boot jump instruction.
.set BS_jmpBoot, 0         ## 3 bytes
 # This can be any string.
.set BS_OEMName, 3         ## 8 bytes (string)
# Count of bytes per sector. 512, 1024, 2048 or 4096. 512 is used.
.set BPB_BytsPerSec, 11    ## 2 bytes
# Number of sectors per allocation unit. 1, 2, 4, 8, 16, 32, 64, or 128 are valid.
# If (BPB_BytsPerSec * BPB_SecPerClus) > 32KiB, the behavior is undefined for most drivers.
.set BPB_SecPerClus, 13    ## 1 byte
# Number of reserved sectors in reserved region. It should not be zero.
# For FAT12/FAT16. this is usually 1, but for FAT32 this is *usually* 32.
.set BPB_ResvdSecCnt, 14   ## 2 bytes
# How many FAT structures there are on the disk. Should always contain at least 2.
.set BPB_NumFATs, 16       ## 1 byte
# For FAT12/16, this contains the count of 32-byte dentries in root.
# For FAT32, this should be set to zero.
.set BPB_RootEntCnt, 17    ## 2 bytes
# This field should be set to zero for FAT32. If this is zero, BPB_TotSec32 needs to be > 0
.set BPB_TotSec16, 19      ## 2 bytes
# This is no longer used. But it is the value for nonremovable media, and is often 0xF8 or 0xF0
.set BPB_Media, 21         ## 1 byte
# BPB_FATSz32 should be used instead for FAT32.
.set BPB_FATSz16, 22       ## 2 bytes
# Sectors per track. For int 0x13. Only for hard disks that are visible on int 0x13.
.set BPB_SecPerTrk, 24     ## 2 bytes
# For int 0x13, head count. A 1.44mb floppy is usually 2.
.set BPB_NumHeads, 26      ## 2 bytes
# Hidden sector count preceding the partition after the current.
.set BPB_HiddSec, 28       ## 4 bytes
# This includes the count of all 4 regions of the disk volume.
.set BPB_TotSec32, 32      ## 4 bytes

# FAT12/16 : FAT32 diversion point. The following do not exist on FAT12/16.

# FAT32 32-bit count of sectors occupied by one FAT.
# Aka, sectors per FAT.
.set BPB_FATSz32, 36       ## 4 bytes
# Bits 0-3: Zero based number of active FAT, valid only when mirroring is disabled
# Bits 4-6: Reserved
# Bit 7: 0 means the FAT is mirrored at runtime. 1 means only 1 is active, referenced in bits 0-3
# Bits 8-15: Reserved
.set BPB_ExtFlags, 40      ## 2 bytes
# Version of the FAT filesystem.
# High byte is major rev, low byte is minor rev.
.set BPB_FSVer, 42         ## 2 bytes
# The cluster number of the first cluster of root dentry.
# Usually, this is at 2.
.set BPB_RootClus, 44      ## 4 bytes
# Sector number of the FSINFO structure in reserved area of the disk.
.set BPB_FSInfo, 48        ## 2 bytes
# If this isn't zero then the sector number in the reserved area is a copy of the boot record.
# Is usually 6.
.set BPB_BkBootSec, 50     ## 2 bytes
# Reserved for future expansion, should be zero.
.set BPB_Reserved, 52      ## 12 bytes
# int 0x13 drive number. Usually 0x80
.set BS_DrvNum, 64         ## 1 byte
# Reserved. Should always be set to zero
.set BS_Reserved1, 65      ## 1 byte

# Extended boot signature, 0x29. Indicates the following 3 field are present.
.set BS_BootSig, 66        ## 1 byte
# Volume serial number. With BS_VolLab, volume tracking on removable media is possible.
# It also allows drivers to detect the wrong disk is inserted. 
# The ID is usually just obtained by combining the current date and time into a 32 bit val.
.set BS_VolID, 67          ## 4 bytes
# Should match the 11 byte volume label in the root directory.
.set BS_VolLab, 71         ## 11 bytes
# Usually irrelevant, but often is set to the FAT type, ie "FAT     "
.set BS_FilSysType, 82     ## 8 bytes



### FAT dentry structure
.set DIR,               0       ## 32 bytes each
.set DIR_Name,          0       ## 11 bytes
.set DIR_Attr,          1       ## byte
.set DIR_FstClusHI,     20      ## word
.set DIR_FstClusLO,     26      ## word
.set DIR_FileSize,      28      ## long
### (only the important parts)
.set DIR_size, 0x20




# In real mode, only certain registers are allowed for memory indexing
# Base: %bx, %bp
# Index: %si, %di
# All base addressing use %ds segment register unless %bp is used,
# in which case it will be %ss.

__start:
        # disk number is in %dl
        cli

        xor     %ax, %ax
        mov     %ax, %ds
        mov     %ax, %es
        mov     %ax, %ss
        movw    $_begin, %sp

        ljmp    $0x0000, $fix_offset_start
fix_offset_start:
        mov     %sp, %bp
        push    %dx     # has %dl as drive number

        # this section gets the anchor to the whole fs. Everything uses it.
first_datasector:
        movl    $_begin, %ebx

        # FstDSec = ResvdSecCnt + (NumFATs * FatSz)
        # FatSz could be either FatSz16 or FatSz32,
        # so a quick test if FATSz16 is zero or not
        # will tell you. If 16 is empty, 32 shouln't be.

        movzwl  BPB_FATSz16(%bx), %eax
        test    %eax, %eax
        jnz     1f
        movl    BPB_FATSz32(%bx), %eax
1:

        movzbl  BPB_NumFATs(%bx), %edi
        mul     %edi
        movzwl  BPB_ResvdSecCnt(%bx), %edx
        addl    %edx, %eax
        movl    %eax, (FstDSec)

        # now that FstDSec is acquired, lets find root with it.
find_root:
        movl    BPB_RootClus(%bx), %edi
        call    clus_to_lba
        mov     %eax, %esi
        mov     $1, %edi
        mov     $0, %cx
        mov     $buffer, %dx
        pop     %ax
        push    %ax
        call    read_disk_lba

        # We have to give up %bp for now
        movl    $buffer, %ebp
        xor     %cx, %cx
.Lread_dir_loop:
        cmpb    $0x00, %ds:DIR_Name(%ebp)
        je      not_found
        cmpb    $0xe5, %ds:DIR_Name(%ebp)
        je      .Lnext
        cmpb    $0x0f, %ds:DIR_Attr(%ebp)
        je      .Lnext

        push    %cx
        lea     DIR_Name(%ebp), %di
        lea     stage2_file, %si
        mov     $stage2_file_len, %cx
        repe    cmpsb
        pop     %cx
        je      load_s2
.Lnext:
        addl    $DIR_size, %ebp
        inc     %cx
        cmp     $16, %cx
        jl      .Lread_dir_loop
        jmp     not_found



.set S2_SEGMENT, 0x1000
.set S2_OFFSET, 0
load_s2:
        movzwl  BPB_BytsPerSec(%bx), %eax
        movzbl  BPB_SecPerClus(%bx), %edi
        mul     %edi
        mov     %eax, %ecx
        mov     %eax, %edi
        movl    %ds:DIR_FileSize(%ebp), %eax
        dec     %edi
        add     %edi, %eax
        div     %ecx
        

        # eax now has how many clusters stage2 is
        push    %eax
        
        movzwl  %ds:DIR_FstClusHI(%ebp), %ecx
        movzwl  %ds:DIR_FstClusLO(%ebp), %edi
        shl     $0x10, %ecx
        or      %ecx, %edi
        ## if %edi is a weird value, its because
        # im not ORing shifted HI onto LO, im doing LO onto shifted HI.

        call    clus_to_lba
        mov     %eax, %esi
        pop     %edi
        mov     $S2_SEGMENT, %cx
        mov     $S2_OFFSET, %dx
        pop     %ax
        push    %ax
        call    read_disk_lba

        movw    $S2_SEGMENT, %ax
        mov     %ax, %ds
        mov     %ax, %es

        pop     %dx

        ljmp    $S2_SEGMENT, $S2_OFFSET

not_found:
disk_err:
        nop
        hlt
        


# Reads from disk via LBA into (buffer)
# Parameters:
#       - %ax: drive number
#       - %edi: clusters to request
#       - %esi: lba
#       - %cx: segment
#       - %dx: buffer
# Returns:
#       - none
# Clobbers: All GP regs except %bp and %bx, %ax
.set READ_LIM, 127
read_disk_lba:
        # most bioses cap out at 127 per request
        # so to make this simpler, we just request 1 at a time
        # this makes the boot slower, but its better than
        # exceeeding the 512 byte limit of this terrible bios-ridden bootsector.
        push    %ax

        movw    %cx, (DAP_segment)
        movw    %dx, (DAP_buffer)
        movl    %esi, (DAP_lba)

        mov     %edi, %ecx
.Lread_loop:
        # movw    $1, (DAP_transfer)
        movzbw  BPB_SecPerClus(%bx), %ax
        mov     %ax, (DAP_transfer)

        test    %cx, %cx
        jz      .Ldone

        
        pop     %dx
        push    %dx

        mov     $DAP, %si
        mov     $0x42, %ah
        int     $BIOS_INT_disk
        jc      disk_err

        dec     %cx
        mov     (DAP_transfer), %ax
        movzbl  BPB_SecPerClus(%bx), %edi
        # mulw    %edi
        addl    %edi, (DAP_lba)
        movl    %edi, %eax
        mulw    BPB_BytsPerSec(%bx)
        addw    %ax, (DAP_buffer)

        jmp     .Lread_loop
.Ldone:
        pop     %ax
        ret
        

# Converts a cluster number to a first-sector LBA number.
# Parameters:
#       - %bx: BS base
#       - %edi: cluster
# Returns:
#       - %eax: lba
# Clobbers: %di, %edx
clus_to_lba:
        subl    $2, %edi
        movzbl  BPB_SecPerClus(%bx), %eax
        mul     %edi
        addl    (FstDSec), %eax
        addl    BPB_HiddSec(%bx), %eax
        ret


# Converts an LBA to a cluster number.
# Parameters:
#       - %bx: BS base
#       - %edi: LBA
# Returns:
#       - %eax: cluster
# Clobbers: %edi, %edx
lba_to_clus:
        subl    (FstDSec), %edi
        mov     %edi, %eax
        subl    BPB_HiddSec(%bx), %eax
        movzbl  BPB_SecPerClus(%bx), %edi
        xor     %edx, %edx
        div     %edi
        addl    $2, %eax
        ret



FstDSec:   .long 0

# The structure needs to be 4-byte aligned.
.align 4
DAP:
DAP_size:       .byte 0x10      # size of packet
DAP_zero:       .byte 0         # always zero
DAP_transfer:   .word 1         # number of sectors to transfer
# The buffer needs to be 2-byte aligned.
DAP_buffer:     .word 0         # Offset
DAP_segment:    .word 0         # Segment
DAP_lba:        .long 0         # LBA, zero is the reserved area, everything else starts from one.
                .long 0         # for big LBA


# puts:
#         push    %dx
#         push    %ax
#         lea     (stage_one_err), %si
# .Lputs_loop:
#         lodsb
#         orb     %al, %al
#         jz      .Lputs_done
#
#         movb    $0x0e, %ah
#         xorb    %bh, %bh
#         int     $BIOS_INT_video
#
#         jmp     .Lputs_loop
# .Lputs_done:
#         pop     %dx
#
#         movb    $0x0e, %ah
#         xorb    %bh, %bh
#         int     $BIOS_INT_video
#
#         pop     %ax
#         pop     %si
#         ret


stage2_file:            .ascii "STAGE2  BIN"
.set stage2_file_len, . - stage2_file
stage_one_err:          .asciz "ERR: "



.fill 510 - (. - fat_jmp), 1, 0
.word 0xaa55

.align 2
buffer:

