#include "boot.h"
#include "kstring.h"
#include "macros.h"
#include "multiboot2.h"
#include "pagetbl.h"
#include "panic.h"
#include "types.h"


extern char _kphys_start[];
extern char _kphys_end[];

struct kpage_core *phys_core = NULL;


static void pfa_region_init()
{
        struct multiboot_tag_mmap *map = multiboot2_tmmap;
        if (unlikely(!map)) {
                panic("GRUB memory map was null!");
        }

        // const u64 kernsz = (u64)_kphys_end - (u64)_kphys_start;

        int i = 0;
        const u8 *end = (u8 *)map + map->size;

        for (u8 *p = (u8 *)map + sizeof(*map); p < end; p += map->entry_size) {
                multiboot_memory_map_t *e = (void *)p;

                if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
                        continue;

                physmem_region_t *r = &phys_core->region[i++];

                /* shouldn't remap the preinit region */
                if ((void *)e->addr == r->base)
                        continue;

                const usize total_regionsz = sizeof(physmem_region_t) * phys_core->regions;
                if (unlikely(offsetof(struct kpage_core, region) + total_regionsz >= PAGESIZE)) {
			panic("system memory mapping has too many regions!");
		}

                r->base = (void *)e->addr;

                /* e->len is probably never malformed, but just in case, we round down */
                r->max_frames = (e->len & ~(PAGESIZE - 1)) / PAGESIZE;
                r->cur_frames = r->max_frames;
                r->top = NULL;

                phys_core->regions++;
        }
}




void kphys_alloc_early_init(void)
{
        extern struct kpage_core preinit_pfa;
        // 0 is preinit region.
        physmem_region_t *ppf = &preinit_pfa.region[0];

        if (unlikely(ppf->max_frames == 0 || ppf->cur_frames == 0)) {
                panic("the impossible happened: zero pages in preinit pages!");
        }


        phys_core = (void *)ppf->top;
        page_frame_t *top = ppf->top - 1;

        memset(phys_core, 1, PAGESIZE);

        physmem_region_t *rg = &phys_core->region[0];

        rg->base = ppf->base;
	rg->top = top;
        rg->max_frames = ppf->max_frames;
        rg->cur_frames = ppf->cur_frames - 1;

	phys_core->regions = 1;
        pfa_region_init();
}




/* only allocates from region 0 of phys_core, the same region used in preinit - *
 * warning: this is an internal call. the pages returned may not be mapped ---- */
void *_kphys_pre_alloc(usize pages)
{
        physmem_region_t *rg = &phys_core->region[0];

        if (pages > rg->cur_frames) 
                return NULL;

        usize pagerq = pages;
        while (pagerq--) {
                rg->top = rg->top->next;
        }

        page_frame_t *a = rg->top;
        rg->cur_frames -= pages;

        return a;
}




/* !!! warning: this is an internal call. the pages returned may not be mapped. !!! */
__attribute__((hot)) //
void *_kphys_alloc(usize pages)
{
        physmem_region_t *rg;
        for (usize i = 0; i < phys_core->regions; i++) {
                rg = &phys_core->region[i];

                if (rg->cur_frames >= pages) {
                        goto alloc;
                }
        }
        return NULL;
alloc:
        usize pagerq = pages;
        while (pagerq--) {
                rg->top = rg->top->next;
        }

        page_frame_t *a = rg->top;
        rg->cur_frames -= pages;

        return a;
}




void kphys_alloc_init(void)
{
        /* TODO: possibly implement a buddy allocator. */
        kphys_alloc_early_init();
}

