//
// Created by SyncShard on 7/27/26.
//

#ifndef KERNEL_PROJECT_STDIO_H
#define KERNEL_PROJECT_STDIO_H

#include "types.h"

extern void putchar(u8 c);
extern void puts(const char *str);

extern void write(const char *str, int len);
extern void clear_line(void);
extern void clear_screen(void);

#endif //KERNEL_PROJECT_STDIO_H
