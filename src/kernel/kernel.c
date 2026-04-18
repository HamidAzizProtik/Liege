#include "stdint.h"
#include "memory/multiboot.h"
#include "memory/pmm.h"
#include "cpu/idt.h"
#include "drivers/keyboard.h"
#include "fonts.h"

/* global graphics state */
static uint32_t* lfb;
static uint32_t* back_buffer;
static uint32_t  screen_width;
static uint32_t  screen_height;
static uint32_t  screen_pitch;

/* plots one pixel into the back buffer */
static inline void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x < screen_width && y < screen_height)
        back_buffer[y * screen_width + x] = color;
}

/* copies back buffer to video memory — one full frame flush */
static void graphics_swap_buffers(void) {
    for (uint32_t y = 0; y < screen_height; y++) {
        volatile uint32_t* line =
            (volatile uint32_t*)((uint8_t*)lfb + (y * screen_pitch));
        for (uint32_t x = 0; x < screen_width; x++)
            line[x] = back_buffer[y * screen_width + x];
    }
}

/* draws a filled rectangle */
static void draw_rect(uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < h; i++)
        for (uint32_t j = 0; j < w; j++)
            put_pixel(x + j, y + i, color);
}

/* draws one character from the bitmap font */
static void draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    unsigned char uc = (unsigned char)c;
    if (uc > 127) return;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 8; j++)
            if (font_bitmap[uc][i] & (0x80 >> j))
                put_pixel(x + j, y + i, color);
}

/* draws a null-terminated string */
static void draw_string(const char* str, uint32_t x,
                        uint32_t y, uint32_t color) {
    while (*str) {
        draw_char(*str++, x, y, color);
        x += 8;
    }
}

/* draws a 32-bit unsigned integer as decimal */
static void draw_uint(uint32_t n, uint32_t x, uint32_t y, uint32_t color) {
    char buf[11];
    buf[10] = '\0';
    int i = 9;

    if (n == 0) {
        draw_char('0', x, y, color);
        return;
    }

    while (n > 0) {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }

    draw_string(&buf[i + 1], x, y, color);
}

/* draws a 32-bit value as hex with 0x prefix */
static void draw_hex(uint32_t n, uint32_t x, uint32_t y, uint32_t color) {
    char digits[] = "0123456789ABCDEF";
    char buf[9];
    buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buf[i] = digits[n & 0xF];
        n >>= 4;
    }
    draw_string("0x", x, y, color);
    draw_string(buf, x + 16, y, color);
}

/* allocates back buffer from PMM — needs screen dimensions set first */
static int graphics_init_backbuffer(void) {
    uint32_t pixels     = screen_width * screen_height;
    uint32_t bytes      = pixels * 4;
    uint32_t pages      = (bytes + 4095) / 4096;

    /* allocate contiguous pages for the back buffer */
    /* store the first page address as our buffer base */
    void* first = pmm_alloc_page();
    if (!first) return 0;

    for (uint32_t i = 1; i < pages; i++) {
        if (!pmm_alloc_page()) return 0;
    }

    back_buffer = (uint32_t*)first;
    return 1;
}

/* kernel entry point */
void kernel_main(uint32_t magic, uint32_t addr) {
    /* initialize interrupt handling before anything else */
    idt_init();
    keyboard_init();
    __asm__ volatile ("sti");

    /* validate multiboot magic */
    if (magic != 0x2BADB002) {
        while (1) __asm__ volatile ("hlt");
    }

    multiboot_info_t* mb = (multiboot_info_t*)addr;

    /* validate framebuffer flag — bit 12 */
    if (!(mb->flags & (1 << 12))) {
        while (1) __asm__ volatile ("hlt");
    }

    /* set up screen dimensions from multiboot */
    lfb           = (uint32_t*)mb->framebuffer_addr_low;
    screen_width  = mb->framebuffer_width;
    screen_height = mb->framebuffer_height;
    screen_pitch  = mb->framebuffer_pitch;

    /* initialize PMM now that we have memory info */
    pmm_init(mb);

    /* allocate back buffer from PMM */
    if (!graphics_init_backbuffer()) {
        while (1) __asm__ volatile ("hlt");
    }

    /* draw background */
    draw_rect(0, 0, screen_width, screen_height, 0x1d1a21);

    /* title bar */
    draw_rect(0, 0, screen_width, 24, 0x2b2730);
    draw_string("Liege OS v0.1.0-alpha", 10, 4, 0xAAAAAA);
    draw_string("tty0", screen_width - 45, 4, 0x555555);

    /* boot status */
    draw_string("[  OK  ] IDT and keyboard initialized",  15, 45, 0x00FF00);
    draw_string("[  OK  ] PMM initialized",               15, 65, 0x00FF00);
    draw_string("[  OK  ] Back buffer allocated",          15, 85, 0x00FF00);

    /* display info — real values from multiboot */
    draw_string("Display:", 15, 120, 0x00A8FF);
    draw_string("Resolution :", 30, 140, 0x777777);
    draw_uint(screen_width,  140, 140, 0xFFFFFF);
    draw_string("x",          140 + (screen_width > 999 ? 32 : 24), 140, 0x777777);
    draw_uint(screen_height, 140 + (screen_width > 999 ? 40 : 32), 140, 0xFFFFFF);

    draw_string("Framebuf   :", 30, 160, 0x777777);
    draw_hex(mb->framebuffer_addr_low, 140, 160, 0xFFFFFF);

    draw_string("BPP        :", 30, 180, 0x777777);
    draw_uint(mb->framebuffer_bpp, 140, 180, 0xFFFFFF);

    /* memory info — real values from multiboot */
    draw_string("Memory:", 15, 210, 0x00A8FF);
    draw_string("Upper Mem  :", 30, 230, 0x777777);
    draw_uint(mb->mem_upper, 140, 230, 0xFFFFFF);
    draw_string("KB", 140 + 48, 230, 0x777777);

    draw_string("Total Pages:", 30, 250, 0x777777);
    draw_uint(pmm_get_total(), 140, 250, 0xFFFFFF);

    draw_string("Used Pages :", 30, 270, 0x777777);
    draw_uint(pmm_get_used(), 140, 270, 0xFFFFFF);

    /* flush to screen */
    graphics_swap_buffers();

    /* wait for input — keyboard is live */
    while (1) {
        char c = keyboard_getchar();
        if (c != 0) {
            /* keyboard is working — placeholder for terminal layer */
            (void)c;
        }
        __asm__ volatile ("hlt");
    }
}