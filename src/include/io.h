//
// Created by SyncShard on 7/25/26.
//

#ifndef KERNEL_PROJECT_IO_H
#define KERNEL_PROJECT_IO_H

#include "types.h"


__attribute__((always_inline, unused)) //
static inline void outb(u16 port, u8 val)
{
	__asm__ volatile (
		"outb %0, %1"
		:
		: "a"(val), "Nd"(port)
		: "memory"
	);
};


__attribute__((always_inline, unused)) //
static inline u8 inb(u16 port)
{
	u8 ret;
	__asm__ volatile (
		"inb %1, %0"
		: "=a"(ret)
		: "Nd"(port)
		: "memory"
	);
	return ret;
};


__attribute__((always_inline, unused)) //
static inline void outw(u16 port, u16 val)
{
	__asm__ volatile (
		"outw %0, %1"
		:
		: "a"(val), "Nd"(port)
		: "memory"
	);
};


__attribute__((always_inline, unused)) //
static inline u16 inw(u16 port)
{
	u16 ret;
	__asm__ volatile (
		"inw %1, %0"
		: "=a"(ret)
		: "Nd"(port)
		: "memory"
	);
	return ret;
};


__attribute__((always_inline, unused)) //
static inline void outl(u16 port, u32 val)
{
	__asm__ volatile (
		"outl %0, %1"
		:
		: "a"(val), "Nd"(port)
		: "memory"
	);
};


__attribute__((always_inline, unused)) //
static inline u32 inl(u16 port)
{
	u32 ret;
	__asm__ volatile (
		"inl %1, %0"
		: "=a"(ret)
		: "Nd"(port)
		: "memory"
	);
	return ret;
};



__attribute__((always_inline, unused)) //
static inline void insb(u16 port, void *out, usize wc)
{
        __asm__ volatile (
                "cld\n\t"
                "rep insb"
                : "+D"(out), "+c"(wc)
                : "d"(port)
                : "memory", "cc"
        );
}
__attribute__((always_inline, unused)) //
static inline void insw(u16 port, void *out, usize wc)
{
        __asm__ volatile (
                "cld\n\t"
                "rep insw"
                : "+D"(out), "+c"(wc)
                : "d"(port)
                : "memory", "cc"
        );
}
__attribute__((always_inline, unused)) //
static inline void insl(u16 port, void *out, usize wc)
{
        __asm__ volatile (
                "cld\n\t"
                "rep insl"
                : "+D"(out), "+c"(wc)
                : "d"(port)
                : "memory", "cc"
        );
}


__attribute__((always_inline, unused)) //
static inline void x86_hlt(void)
{
	__asm__ volatile("hlt" ::: "memory");
}

#endif //KERNEL_PROJECT_IO_H
