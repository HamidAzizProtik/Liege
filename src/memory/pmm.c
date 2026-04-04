#include "pmm.h"

#define PAGE_SIZE 4096

static uint8_t* bitmap;
static uint32_t total_pages;

extern uint8_t _kernel_end;

#define BIT_INDEX(x) ((x) / 8)
#define BIT_OFFSET(x) ((x) % 8)

static void set_bit(uint32_t bit)
{
    bitmap[BIT_INDEX(bit)] |= (1 << BIT_OFFSET(bit));
}

static void clear_bit(uint32_t bit)
{
    bitmap[BIT_INDEX(bit)] &= ~(1 << BIT_OFFSET(bit));
}

static int test_bit(uint32_t bit)
{
    return bitmap[BIT_INDEX(bit)] & (1 << BIT_OFFSET(bit));
}

void pmm_init(multiboot_info_t* mb)
{
    total_pages = (mb->mem_upper * 1024) / PAGE_SIZE;

    bitmap = (uint8_t*)&_kernel_end;

    for (uint32_t i = 0; i < total_pages; i++)
        set_bit(i);

    /* mark lower memory as usable (simple safe assumption) */
    for (uint32_t i = 0; i < total_pages / 2; i++)
        clear_bit(i);

    /* protect kernel + bitmap memory */
    uint32_t kernel_pages = ((uint32_t)&_kernel_end) / PAGE_SIZE;

    for (uint32_t i = 0; i <= kernel_pages + 1; i++)
        set_bit(i);
}

void* pmm_alloc_page()
{
    for (uint32_t i = 0; i < total_pages; i++)
    {
        if (!test_bit(i))
        {
            set_bit(i);
            return (void*)(i * PAGE_SIZE);
        }
    }

    return 0;
}

void pmm_free_page(void* addr)
{
    uint32_t index = (uint32_t)addr / PAGE_SIZE;

    if (index >= total_pages)
        return;

    clear_bit(index);
}