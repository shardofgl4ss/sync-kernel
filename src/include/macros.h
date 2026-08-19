#ifndef SYOS_MACROS_H
#define SYOS_MACROS_H

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

#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)

#define _SY_PRIMITIVE           __attribute__((always_inline)) static inline


#endif

#endif // SYOS_MACROS_H
