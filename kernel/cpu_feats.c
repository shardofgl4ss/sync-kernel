// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Sync Shard

#include "cpu_feats.h"

#include "types.h"
#include <cpuid.h>


typedef struct cpu_leaf_lim {
        u32 max_std_leaf;
        u32 max_ext_leaf;
} cpu_leaf_limits_t;

typedef struct cpu_leaf {
        union {
                u32 regs[4];
                
                struct {
                        u32 eax;
                        u32 ebx;
                        u32 ecx;
                        u32 edx;
                };
        };

        bool supported;
} cpu_leaf_t;

typedef struct cpu_info {
        cpu_leaf_limits_t limits;

        cpu_leaf_t leaf_1;
        cpu_leaf_t leaf_7;
        cpu_leaf_t leaf_ext;
} cpu_info_t;


static cpu_info_t boot_cpu = {};


static void cpu_assign_from_leaf(cpu_leaf_t *l, u32 *val, enum cpuid_reg reg)
{
        switch (reg) {
        case CPUID_REG_EBX:
                *val = l->ebx;
                return;
        case CPUID_REG_ECX:
                *val = l->ecx;
                return;
        case CPUID_REG_EDX:
                *val = l->edx;
                return;
        default:
                return;
        };
}




void cpu_get_capabilities(void)
{
        u32 _;
        cpu_leaf_t *leaf;

        cpu_leaf_limits_t *lim = &boot_cpu.limits;


        __cpuid(0, lim->max_std_leaf, _, _, _);
        __cpuid(0x80000000, lim->max_ext_leaf, _, _, _);


        if (lim->max_std_leaf >= 1) {
                leaf = &boot_cpu.leaf_1;
                __cpuid(1, leaf->eax, leaf->ebx, leaf->ecx, leaf->edx);
                leaf->supported = true;
        }


        if (lim->max_std_leaf >= 7) {
                leaf = &boot_cpu.leaf_7;
                __cpuid_count(7, 0, leaf->eax, leaf->ebx, leaf->ecx, leaf->edx);
                leaf->supported = true;
        }
        

        if (lim->max_ext_leaf >= CPUID_EXT_LEAF) {
                leaf = &boot_cpu.leaf_ext;
                __cpuid(CPUID_EXT_LEAF, leaf->eax, leaf->ebx, leaf->ecx, leaf->edx);
                leaf->supported = true;
        }
}




bool cpu_has_feat(enum cpuid_feats feat)
{
        enum cpuid_reg reg = (feat >> 5) & 0x07;
        u32 leaf = (feat >> 8) & 0xFF;
        u32 bit = feat & 0x1F;

        u32 val = {};


        switch (leaf) {
        case 1:
                if (!boot_cpu.leaf_1.supported) return false;
                cpu_assign_from_leaf(&boot_cpu.leaf_1, &val, reg);
                break;
        case 7:
                if (!boot_cpu.leaf_7.supported) return false;
                cpu_assign_from_leaf(&boot_cpu.leaf_7, &val, reg);
                break;
        case 2:
                if (!boot_cpu.leaf_ext.supported) return false;
                cpu_assign_from_leaf(&boot_cpu.leaf_ext, &val, reg);
                break;
        default:
                return false;
        }

        return (val & (1U << bit)) != 0;
}

