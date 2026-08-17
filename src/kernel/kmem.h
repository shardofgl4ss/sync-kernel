#ifndef _KERNEL_KMEM_H
#define _KERNEL_KMEM_H

#include "types.h"


#define _PAGEALIGNED            __attribute__((aligned(PAGESIZE)))
#define PAGESIZE                4096
#define PAGE_ALIGNUP(x)         (((x) + (PAGESIZE - 1)) & ~(PAGESIZE - 1))
#define PAGE_ALIGNDOWN(x)       ((x) & ~(PAGESIZE - 1))
//
// the kernel just dies if there are more then 256 memory regions.
// it shouldn't ever happen anyway, even in huge memory systems.
#define KPG_MAX_REGIONS         4
#define REGION_TERMINATOR       ((void *)-1)



typedef struct _kpage_frame {
        struct page_frame_t *next;
} _PAGEALIGNED page_frame_t;

typedef struct phys_mem_rpool {
        u64 region_size[sizeof(u64) * KPG_MAX_REGIONS];
        u64 region_addr[sizeof(u64) * KPG_MAX_REGIONS];
        // bitmap of each region for use in above arrays.
        u64 region_bmap[KPG_MAX_REGIONS];
        // actual region max
        int regions;
} phys_rpool_t;

typedef struct kmem_region {
        page_frame_t *top;
        u32 max_frames;
        u32 region;
} memory_region_t;

struct kpage_core {
        struct phys_mem_rpool phypool;
        memory_region_t region[];
} _PAGEALIGNED;

_Static_assert(sizeof(struct kpage_core) == PAGESIZE,
                "kpage_core not equal to size of page!\n");

extern struct kpage_core *PHYS_CORE;


/* kmem early init relies on an already mapped address space. */
extern void kmem_early_pf_init(void *pa_base, u64 bytes);
extern void kmem_pf_init(void);



#endif // _KERNEL_KMEM_H
