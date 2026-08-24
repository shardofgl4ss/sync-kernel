// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once


#ifndef __ASSEMBLER__
// Non-assembler macros start here.

#define likely(x)               __builtin_expect(!!(x), 1)
#define unlikely(x)             __builtin_expect(!!(x), 0)


/* https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html */


#define _const_                 __attribute__((const))
#define _pure_                  __attribute__((pure))
#define _force_inline_          __attribute__((always_inline)) inline
#define _no_inline_             __attribute__((noinline))
#define _noreturn_              __attribute__((noreturn))
#define _packed_                __attribute__((packed))
#define _nonstring_             __attribute__((nonstring))
#define _no_ipa_                __attribute__((noipa))
#define _externally_visible_    __attribute__((externally_visible))
#define _no_stack_protector_    __attribute__((no_stack_protector))
#define _fallthrough_           __attribute__((fallthrough))
#define _hot_                   __attribute__((hot))
#define _cold_                  __attribute__((cold))
#define _transparent_union_     __attribute__((transparent_union))
#define _naked_                 __attribute__((naked))
#define _artificial_            __attribute__((artificial))

/* C23 specific ones. */
#define _unsequenced_           __attribute__((unsequenced))
#define _reproducible_          __attribute__((reproducible))

#define _unused_                __attribute__((unused))
#define _used_                  __attribute__((used))

#define _malloc_                __attribute__((malloc))
#define _malloc_arg_(__VA_ARGS__)       __attribute__((malloc(__VA_ARGS__)))
#define _aligned_(x)            __attribute__((aligned(x)))
#define _alloc_align_(x)        __attribute__((alloc_align(x)))
#define _assume_aligned_(__VA_ARGS__)   __attribute__((assume_aligned(__VA_ARGS__)))
#define _nonnull_(__VA_ARGS__)  __attribute__((nonnull(__VA_ARGS__)))

#define _SY_PRIMITIVE           __attribute__((always_inline)) static inline

#define _preinit_               __attribute__((section(".preinit.text")))
#define _preinit_uninit_        __attribute__((section(".preinit.bss")))
#define _preinit_data_          __attribute__((section(".preinit.data")))


#else
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
#endif

