//
// Created by SyncShard on 7/26/26.
//

#ifndef KERNEL_PROJECT_IRQ_H
#define KERNEL_PROJECT_IRQ_H


__attribute__((always_inline)) static inline void x86_cli(void)   { __asm__ volatile ("cli" ::: "memory"); }
__attribute__((always_inline)) static inline void x86_sti(void)  { __asm__ volatile ("sti" ::: "memory"); }

#endif //KERNEL_PROJECT_IRQ_H
