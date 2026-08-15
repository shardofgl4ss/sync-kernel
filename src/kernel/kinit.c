//
// Created by SyncShard on 7/25/26.
//

// #include "kinit.h"
//
// #include "idt.h"
// #include "isr.h"
// #include "kstring.h"
// #include "vga.h"

// extern char __bss_start;
// extern char __bss_end;
//
// void clear_kernel_bss(void);
//
// static void (*kinit_table[])(void) = {
// 	clear_kernel_bss,
// 	x64_isr_init,
// 	idt_init,
// 	vga_init,
// };
//
// static constexpr size_t KINIT_ENTRIES = sizeof(kinit_table) / sizeof(*kinit_table);
//
// void kinit(void)
// {
// 	for (size_t i = 0; i < KINIT_ENTRIES; i++) {
// 		(kinit_table[i])();
// 	}
// }
//
//
// void clear_kernel_bss(void)
// {
//         if (&__bss_end == &__bss_start)
//                 return;
//
//         const intptr_t bss_size = ((u8 *)&__bss_end - (u8 *)&__bss_start);
//
//         if (bss_size <= 0)
//                 return;
//
//         memset(&__bss_start, 0, bss_size);
// }
//
