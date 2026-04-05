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

    /* mark all upper memory as free initially */
    for (uint32_t i = 0; i < total_pages; i++)
        clear_bit(i);

    /* protect kernel memory (from 1MB) */
    uint32_t kernel_pages = ((uint32_t)&_kernel_end - 0x100000) / PAGE_SIZE;

    /* protect bitmap memory */
    uint32_t bitmap_size = (total_pages + 7) / 8;
    uint32_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = 0; i <= kernel_pages + bitmap_pages; i++)
        set_bit(i);
}

void* pmm_alloc_page()
{
    for (uint32_t i = 0; i < total_pages; i++)
    {
        if (!test_bit(i))
        {
            set_bit(i);
            return (void*)((i + 256) * PAGE_SIZE);  /* 256 * 4096 = 1MB */
        }
    }

    return 0;
}

void pmm_free_page(void* addr)
{
    uint32_t index = ((uint32_t)addr / PAGE_SIZE) - 256;

    if (index >= total_pages)
        return;

    clear_bit(index);
}