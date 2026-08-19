#ifndef _KERNEL_PAGETBL_H
#define _KERNEL_PAGETBL_H

#include "types.h"
#include "macros.h"
#include "kmem.h"

typedef struct x86_page_tbl {
        u64 entries[512];
} _PAGEALIGNED PageTable;

static_assert(sizeof(PageTable) == 0x1000, __FILE__ " PageTable size error\n");

extern char             KERNEL_OFFSET[];
#define KPAGE_ERR               ((void *)-1)
#define PHYSMAP         ((u64)KERNEL_OFFSET)



typedef enum {
	PT_FLAG_PRESENT                 = 1 << 0,
	PT_FLAG_WRITABLE                = 1 << 1,
	PT_FLAG_USER                    = 1 << 2,
	PT_FLAG_WRITE_THROUGH           = 1 << 3,
	PT_FLAG_PAGECACHE_DISABLE       = 1 << 4,
	PT_FLAG_DIRTY                   = 1 << 5,
        PT_FLAG_PAGESIZE                = 1 << 7,
} PT_FLAG_BITS;


enum addrspace_alloc_type {
	ADDRSPACE_ALLOC_TYPE_4K = 0,
	// 2MiB pages.
	ADDRSPACE_ALLOC_TYPE_2M = 1,
	// 1GiB pages.
	ADDRSPACE_ALLOC_TYPE_1G = 2,
};


static constexpr int KERN_START_MEMB = 131072;
extern u64 LOAD_ADDR;


/* maps a region of size bytes,                 *
 * returns KPAGE_ERR on error,                  *
 * or a valid ptr to address space on success.  *
 * phys is rounded down to nearest page size,   *
 * bytes is rounded up to multiple of pagesize. *
 * zero is a valid type for standard pages.     */
extern void *kmap(void *phys, void *virt, u64 bytes, enum addrspace_alloc_type type);

/* linear mapping only. */
__attribute__((const)) //
static inline void *virt_to_phys(void *v) { return (void *)((u64)v - PHYSMAP); }
/* linear mapping only. */
__attribute__((const)) //
static inline void *phys_to_virt(void *p) { return (void *)((u64)p + PHYSMAP); }


extern void kalloc_init(void);


#endif // _KERNEL_PAGETBL_H
