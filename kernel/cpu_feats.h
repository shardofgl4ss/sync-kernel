// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#pragma once

#include "types.h"

#include <cpuid.h>

enum {
	CPUID_L1_ECX_SSE3	  = 1 << 0,
	CPUID_L1_ECX_PCLMUL	  = 1 << 1,
	CPUID_L1_ECX_DTES64	  = 1 << 2,
	CPUID_L1_ECX_MONITOR	  = 1 << 3,
	CPUID_L1_ECX_DS_CPL	  = 1 << 4,
	CPUID_L1_ECX_VMX	  = 1 << 5,
	CPUID_L1_ECX_SMX	  = 1 << 6,
	CPUID_L1_ECX_EST	  = 1 << 7,
	CPUID_L1_ECX_TM2	  = 1 << 8,
	CPUID_L1_ECX_SSSE3	  = 1 << 9,
	CPUID_L1_ECX_CID	  = 1 << 10,
	CPUID_L1_ECX_SDBG	  = 1 << 11,
	CPUID_L1_ECX_FMA	  = 1 << 12,
	CPUID_L1_ECX_CX16	  = 1 << 13,
	CPUID_L1_ECX_XTPR	  = 1 << 14,
	CPUID_L1_ECX_PDCM	  = 1 << 15,
	CPUID_L1_ECX_PCID	  = 1 << 17,
	CPUID_L1_ECX_DCA	  = 1 << 18,
	CPUID_L1_ECX_SSE4_1	  = 1 << 19,
	CPUID_L1_ECX_SSE4_2	  = 1 << 20,
	CPUID_L1_ECX_X2APIC	  = 1 << 21,
	CPUID_L1_ECX_MOVBE	  = 1 << 22,
	CPUID_L1_ECX_POPCNT	  = 1 << 23,
	CPUID_L1_ECX_TSC_DEADLINE = 1 << 24,
	CPUID_L1_ECX_AES	  = 1 << 25,
	CPUID_L1_ECX_XSAVE	  = 1 << 26,
	CPUID_L1_ECX_OSXSAVE	  = 1 << 27,
	CPUID_L1_ECX_AVX	  = 1 << 28,
	CPUID_L1_ECX_F16C	  = 1 << 29,
	CPUID_L1_ECX_RDRAND	  = 1 << 30,
	CPUID_L1_ECX_HYPERVISOR	  = 1 << 31,

	CPUID_L1_EDX_FPU     = 1 << 0,
	CPUID_L1_EDX_VME     = 1 << 1,
	CPUID_L1_EDX_DE	     = 1 << 2,
	CPUID_L1_EDX_PSE     = 1 << 3,
	CPUID_L1_EDX_TSC     = 1 << 4,
	CPUID_L1_EDX_MSR     = 1 << 5,
	CPUID_L1_EDX_PAE     = 1 << 6,
	CPUID_L1_EDX_MCE     = 1 << 7,
	CPUID_L1_EDX_CX8     = 1 << 8,
	CPUID_L1_EDX_APIC    = 1 << 9,
	CPUID_L1_EDX_SEP     = 1 << 11,
	CPUID_L1_EDX_MTRR    = 1 << 12,
	CPUID_L1_EDX_PGE     = 1 << 13,
	CPUID_L1_EDX_MCA     = 1 << 14,
	CPUID_L1_EDX_CMOV    = 1 << 15,
	CPUID_L1_EDX_PAT     = 1 << 16,
	CPUID_L1_EDX_PSE36   = 1 << 17,
	CPUID_L1_EDX_PSN     = 1 << 18,
	CPUID_L1_EDX_CLFLUSH = 1 << 19,
	CPUID_L1_EDX_DS	     = 1 << 21,
	CPUID_L1_EDX_ACPI    = 1 << 22,
	CPUID_L1_EDX_MMX     = 1 << 23,
	CPUID_L1_EDX_FXSR    = 1 << 24,
	CPUID_L1_EDX_SSE     = 1 << 25,
	CPUID_L1_EDX_SSE2    = 1 << 26,
	CPUID_L1_EDX_SS	     = 1 << 27,
	CPUID_L1_EDX_HTT     = 1 << 28,
	CPUID_L1_EDX_TM	     = 1 << 29,
	CPUID_L1_EDX_IA64    = 1 << 30,
	CPUID_L1_EDX_PBE     = 1 << 31,

	CPUID_L7_EBX_FSGSBASE = 1 << 0,
	CPUID_L7_EBX_BMI1     = 1 << 3,
	CPUID_L7_EBX_AVX2     = 1 << 5,
	CPUID_L7_EBX_BMI2     = 1 << 8,
	CPUID_L7_EBX_ERMS     = 1 << 9,
	CPUID_L7_EBX_INVPCID  = 1 << 10,
	CPUID_L7_EBX_AVX512F  = 1 << 16,
	CPUID_L7_EBX_RDSEED   = 1 << 18,
	CPUID_L7_EBX_ADX      = 1 << 19,
	CPUID_L7_EBX_SMAP     = 1 << 20,

	CPUID_L7_ECX_AVX512_VBMI = 1 << 1,
	CPUID_L7_ECX_UMIP	 = 1 << 2,
	CPUID_L7_ECX_PKU	 = 1 << 3,
	CPUID_L7_ECX_OSPKE	 = 1 << 4,

	CPUID_L7_EDX_MD_CLEAR		    = 1 << 8,
	CPUID_L7_EDX_IBPB		    = 1 << 26,
	CPUID_L7_EDX_STIBP		    = 1 << 27,
	CPUID_L7_EDX_L1D_FLUSH		    = 1 << 28,
	CPUID_L7_EDX_IA32_ARCH_CAPABILITIES = 1 << 29,
	CPUID_L7_EDX_SSBD		    = 1 << 31,

	CPUID_EXT_ECX_LAHF_LM	 = 1 << 20,
	CPUID_EXT_ECX_CMPXCHG16B = 1 << 20,

	CPUID_EXT_EDX_SYSCALL = 1 << 11,
	CPUID_EXT_EDX_NX      = 1 << 20,
	CPUID_EXT_EDX_PDPE1G  = 1 << 26,
	CPUID_EXT_EDX_RDTSCP  = 1 << 27,
	CPUID_EXT_EDX_LM      = 1 << 29,

};

static constexpr u32 CPUID_EXT_LEAF = 0x80000001;

enum cpuid_reg {
        CPUID_REG_EBX = 0,
        CPUID_REG_ECX,
        CPUID_REG_EDX,
};

/* 
 * [0:4] Bit offset
 * [5:7] Register
 * [8:15] Leaf
 */
#define CPUID_FEAT_GROUP(leaf, reg, bit) \
        ((u16)(((leaf) << 7) | ((reg) << 5 | (bit))))

enum cpuid_feats : u16 {
	/* leaf 1 */
	CPUID_FEAT_TSC	= CPUID_FEAT_GROUP(1, CPUID_REG_EDX, 4),
	CPUID_FEAT_PAE	= CPUID_FEAT_GROUP(1, CPUID_REG_EDX, 6),
	CPUID_FEAT_APIC = CPUID_FEAT_GROUP(1, CPUID_REG_EDX, 9),

	CPUID_FEAT_PCID		= CPUID_FEAT_GROUP(1, CPUID_REG_ECX, 17),
	CPUID_FEAT_X2APIC	= CPUID_FEAT_GROUP(1, CPUID_REG_ECX, 21),
	CPUID_FEAT_TSC_DEADLINE = CPUID_FEAT_GROUP(1, CPUID_REG_ECX, 24),

	/* 7 */
	CPUID_FEAT_FSGSBASE = CPUID_FEAT_GROUP(7, CPUID_REG_EBX, 0),
	CPUID_FEAT_AVX2	    = CPUID_FEAT_GROUP(7, CPUID_REG_EBX, 5),
	CPUID_FEAT_SMAP	    = CPUID_FEAT_GROUP(7, CPUID_REG_EBX, 20),

	/* ext, we use 2 here for the bit pack */
	CPUID_FEAT_SYSCALL = CPUID_FEAT_GROUP(2, CPUID_REG_EDX, 11),
	CPUID_FEAT_NX	   = CPUID_FEAT_GROUP(2, CPUID_REG_EDX, 20),
	CPUID_FEAT_PDPT1G  = CPUID_FEAT_GROUP(2, CPUID_REG_EDX, 26),
	CPUID_FEAT_LM	   = CPUID_FEAT_GROUP(2, CPUID_REG_EDX, 29),
};


extern void cpu_get_capabilities(void);

static inline int cpu_get_model(void)
{
        i32 ebx, _;
        __cpuid(0, _, ebx, _, _);
        return ebx;
}

