#ifndef PMM_H
#define PMM_H

#include "stdint.h"
#include "memory/multiboot.h"

/* initializes the physical memory manager using the multiboot memory map */
void pmm_init(multiboot_info_t* mb);

/* allocates one 4KB page — returns its physical address or 0 if out of memory */
void* pmm_alloc_page(void);

/* frees a previously allocated page */
void pmm_free_page(void* addr);

/* returns total number of pages tracked */
uint32_t pmm_get_total(void);

/* returns number of used pages */
uint32_t pmm_get_used(void);

#endif