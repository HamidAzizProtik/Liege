#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include "multiboot.h"

void pmm_init(multiboot_info_t* mb);
void* pmm_alloc_page();
void pmm_free_page(void* addr);

#endif