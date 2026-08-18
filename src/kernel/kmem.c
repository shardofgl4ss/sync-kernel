#include "types.h"
#include "pagetbl.h"
#include "kmem.h"
#include "panic.h"

struct kpage_core *PHYS_CORE = NULL;



void kmem_early_pf_init(void *pa_base, u64 bytes)
{
        if (!PHYS_CORE) { 
                panic(__FILE__": physical memory preinit structure invalid at preinit");
        }
        page_frame_t *pa = (void *)PAGE_ALIGNDOWN((u64)pa_base);
        u64 aligned_bytes = PAGE_ALIGNUP(bytes);

        PHYS_CORE->region->idx = 0;
        // kfree_frame_range(&PHYS_CORE->region[0], pa, aligned_bytes / PAGESIZE);
}


void kmem_pf_init(void)
{

}

