#ifndef _KERNEL_PAGETBL_H
#define _KERNEL_PAGETBL_H

#include "types.h"
#include "macros.h"

typedef struct x86_page_tbl {
        qword entries[512];
} __attribute__((aligned(0x1000))) PageTable;

static_assert(sizeof(PageTable) == 0x1000, __FILE__ " PageTable size error\n");

#define KPAGE_ERR    ((void *)-1)


typedef enum {
	PT_FLAG_PRESENT                 = 1 << 0,
	PT_FLAG_WRITABLE                = 1 << 1,
	PT_FLAG_USER                    = 1 << 2,
	PT_FLAG_WRITE_THROUGH           = 1 << 3,
	PT_FLAG_PAGECACHE_DISABLE       = 1 << 4,
	PT_FLAG_DIRTY                   = 1 << 5,
        PT_FLAG_PAGESIZE                = 1 << 7,
} PT_FLAG_BITS;



static constexpr usize PAGE_PT_COUNT = 8;


extern PageTable KPAGETBL_L4;
extern PageTable KPAGETBL_L3;
extern PageTable KPAGETBL_L2;
extern PageTable KPAGETBL_L1[PAGE_PT_COUNT];


extern u64 LOAD_ADDR;

extern void kpage_init(void);


#endif // _KERNEL_PAGETBL_H
