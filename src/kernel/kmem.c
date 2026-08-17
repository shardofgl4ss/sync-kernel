#include "types.h"
#include "kmem.h"
#include "panic.h"

struct kpage_core *PHYS_CORE = NULL;



void kmem_early_pf_init(void *pa_base, u64 bytes)
{
        if (!PHYS_CORE) { 
                panic(__FILE__":"__LINE__ " physical memory preinit structure invalid at preinit");
        }
        page_frame_t *pa = (void *)PAGE_ALIGNDOWN((u64)pa_base);
        u64 aligned_bytes = PAGE_ALIGNUP(bytes);

        for (usize i = 0; i < aligned_bytes / PAGESIZE; i++) {
                
        }
}


void kmem_pf_init(void)
{

}

