#ifndef _KERNEL_PFRAME_H
#define _KERNEL_PFRAME_H

#include "types.h"
#include "pagetbl.h"


typedef struct _kpage_frame {
        struct _kpage_frame *next;
} _PAGEALIGNED page_frame_t;

typedef struct kphysregion {
        void *base;
        page_frame_t *top;
        u64 max_frames;
        u64 cur_frames;
} physmem_region_t;

struct kpage_core {
        usize usable_ram;
        usize regions;
        physmem_region_t region[];
} _PAGEALIGNED;

_Static_assert(sizeof(struct kpage_core) == PAGESIZE,
                "kpage_core not equal to size of page!\n");

extern struct kpage_core *phys_core;



extern void kphys_alloc_init(void);

/* allocates a count of physical pages, returns a physical address ptr. */
__attribute__((hot)) //
extern void *_kphys_alloc(usize pages);

#endif //_KERNEL_PFRAME_H
