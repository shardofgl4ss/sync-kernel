/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Sync Shard
 */

.extern CODE_SEGMENT64
.type CODE_SEGMENT64, @object

.extern panic
.type panic, @function


.section .text

.globl gdt_asm_init
.type gdt_asm_init, @function
gdt_asm_init:
        .cfi_startproc
        push    %rbp
        mov     %rsp, %rbp
        test    %rdi, %rdi
        jz      .Lerr
        
        lgdt    (%rdi)
        pushq   CODE_SEGMENT64(%rip)
        leaq    after(%rip), %rax
        pushq   %rax

        lretq
.Lerr:
        lea     (gdt_init_err_str), %rdi
        call    panic
        hlt

after:
        leave
        ret
        .cfi_endproc
.size gdt_asm_init, . - gdt_asm_init


.section .rodata
gdt_init_err_str:       .asciz __FILE__ ": gdt_asm_init() received null GDT ptr!!\n"
