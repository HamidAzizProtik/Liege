#include "memory/pmm.h"

#define PAGE_SIZE 4096

static uint8_t* bitmap;
static uint32_t total_pages;

/* symbol defined by linker — marks end of kernel binary in memory */
extern uint8_t _kernel_end;

#define BIT_INDEX(x)  ((x) / 8)
#define BIT_OFFSET(x) ((x) % 8)

/* marks page as used */
static void set_bit(uint32_t bit) {
    bitmap[BIT_INDEX(bit)] |= (1 << BIT_OFFSET(bit));
}

/* marks page as free */
static void clear_bit(uint32_t bit) {
    bitmap[BIT_INDEX(bit)] &= ~(1 << BIT_OFFSET(bit));
}

/* returns 1 if page is used, 0 if free */
static int test_bit(uint32_t bit) {
    return bitmap[BIT_INDEX(bit)] & (1 << BIT_OFFSET(bit));
}

/* initializes the PMM — places bitmap right after kernel, marks kernel pages used */
void pmm_init(multiboot_info_t* mb) {
    total_pages = (mb->mem_upper * 1024) / PAGE_SIZE;

    /* bitmap lives immediately after the kernel binary */
    bitmap = (uint8_t*)&_kernel_end;

    /* mark all pages as free initially */
    for (uint32_t i = 0; i < total_pages; i++)
        clear_bit(i);

    /* calculate how many pages the kernel occupies from 1MB */
    uint32_t kernel_pages = ((uint32_t)&_kernel_end - 0x100000) / PAGE_SIZE;

    /* calculate how many pages the bitmap itself occupies */
    uint32_t bitmap_size  = (total_pages + 7) / 8;
    uint32_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

    /* protect kernel and bitmap pages from being allocated */
    for (uint32_t i = 0; i <= kernel_pages + bitmap_pages; i++)
        set_bit(i);
}

/* allocates one 4KB page — returns physical address or 0 if none free */
void* pmm_alloc_page(void) {
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!test_bit(i)) {
            set_bit(i);
            return (void*)((i + 256) * PAGE_SIZE); /* +256 skips first 1MB */
        }
    }
    return 0; /* out of memory */
}

/* frees a previously allocated page */
void pmm_free_page(void* addr) {
    uint32_t index = ((uint32_t)addr / PAGE_SIZE) - 256;
    if (index >= total_pages) return;
    clear_bit(index);
}

/* returns total pages tracked by the PMM */
uint32_t pmm_get_total(void) {
    return total_pages;
}

/* returns number of used pages by counting set bits */
uint32_t pmm_get_used(void) {
    uint32_t used = 0;
    for (uint32_t i = 0; i < total_pages; i++) {
        if (test_bit(i)) used++;
    }
    return used;
}