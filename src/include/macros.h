#ifndef SHARED_INC_MACROS_H
#define SHARED_INC_MACROS_H

#ifdef __ASSEMBLER__
// Assembler-specific macros start here.


.macro mpush regs:vararg
        .irp reg,\regs
                push    \reg
        .endr
.endm
.macro mpop regs:vararg
        .irp reg,\regs
                pop     \reg
        .endr
.endm


.macro defstr name,text
        \name:  .asciz \text
        .set    \name\()_len, . - \name - 1
.endm


.macro cpymem dst, src, len
        lea     \dst, %rdi
        lea     \src, %rsi
        mov     \len, %rcx
        cld
        rep     movsb
.endm

.macro setmem dst, c, len
        xor     %rax, %rax
        mov     \val, %al
        lea     \dst, %rdi
        mov     \len, %rcx
        cld
        rep     stosb
.endm


#else
// Non-assembler macros start here.


#endif

#endif // SHARED_INC_MACROS_H
