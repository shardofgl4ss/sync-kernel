#ifndef _KERNEL_PAGETBL_H
#define _KERNEL_PAGETBL_H

#include "types.h"
#include "macros.h"
#include "kmem.h"

typedef struct x86_page_tbl {
        u64 entries[512];
} _PAGEALIGNED PageTable;

static_assert(sizeof(PageTable) == 0x1000, __FILE__ " PageTable size error\n");

#define KPAGE_ERR               ((void *)-1)



typedef enum {
	PT_FLAG_PRESENT                 = 1 << 0,
	PT_FLAG_WRITABLE                = 1 << 1,
	PT_FLAG_USER                    = 1 << 2,
	PT_FLAG_WRITE_THROUGH           = 1 << 3,
	PT_FLAG_PAGECACHE_DISABLE       = 1 << 4,
	PT_FLAG_DIRTY                   = 1 << 5,
        PT_FLAG_PAGESIZE                = 1 << 7,
} PT_FLAG_BITS;



static constexpr int KERN_START_MEMB    = 131072;
extern u64 LOAD_ADDR;

extern void kpage_init(void *mmap);


#endif // _KERNEL_PAGETBL_H
