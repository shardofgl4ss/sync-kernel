//
// Created by SyncShard on 7/25/26.
//

#include "idt.h"
#include "ktypes.h"

#define EXTRACT_LOW_16(x)       ((u16)((usize)(x) & 0xFFFF))
#define EXTRACT_MID_16(x)       ((u16)(((usize)(x) >> 16) & 0xFFFF))
#define EXTRACT_HI_32(x)        ((u32)((usize)(x) >> 32))

typedef struct {
	u16 offs_lo;
	u16 segment_selector;
	u8 ist;
	u8 flags;
	u16 offs_mid;
	u32 offs_hi;
	u32 _reserved;
} __attribute__((packed)) idt64_entry;

_Static_assert(sizeof(idt64_entry) == 0x10, "IDT entry size error\n");

typedef struct {
	u16 limit;
	idt64_entry *base;
} __attribute__((packed)) interrupt_descriptor_64;

_Static_assert(sizeof(interrupt_descriptor_64) == 10, "IDT entry size error\n");

idt64_entry IDT_arr[256];
interrupt_descriptor_64 _IDT_descriptor = {
	.limit = (u16)sizeof(IDT_arr) - 1,
	.base = IDT_arr,
};


__attribute__((always_inline, nonnull(1)))
static inline void x64_idt_load(interrupt_descriptor_64 *idt)
{
	__asm__ volatile ("lidt	%0"
		:
		: "m" (*idt)
		: "memory"
	);
}


void idt64_setgate(const int interrupt,
                   const void *base,
                   const u16 seg_desc,
                   const u8 flags)
{
	idt64_entry *idt = &IDT_arr[interrupt];

	// u16 offs lo, u16 offs mid, // u32 offs hi

	idt->offs_lo = EXTRACT_LOW_16(base);
	idt->offs_mid = EXTRACT_MID_16(base);
	idt->offs_hi = EXTRACT_HI_32(base);

	idt->flags = flags;
	idt->segment_selector = seg_desc;
	idt->ist = 0;
	idt->_reserved = 0;
}


void idt64_enablegate(const int interrupt)
{
	FLAGSET(IDT_arr[interrupt].flags, F_IDT_PRESENT);
}


void idt64_disablegate(const int interrupt)
{
	FLAGUNSET(IDT_arr[interrupt].flags, F_IDT_PRESENT);
}


void idt_init(void)
{
	x64_idt_load(&_IDT_descriptor);
}
