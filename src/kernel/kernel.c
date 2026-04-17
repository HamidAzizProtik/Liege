#include <stdint.h>
#include "memory/multiboot.h"
#include "fonts.h"

/* Global Graphics State */
uint32_t* lfb;            
uint32_t* back_buffer;    
uint32_t  screen_width;
uint32_t  screen_height;
uint32_t  screen_pitch;

/* Optimized Pixel Plotter */
inline void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x < screen_width && y < screen_height) {
        back_buffer[y * screen_width + x] = color;
    }
}

/* Hardware Flush */
void graphics_swap_buffers() {
    volatile uint32_t* dest = (volatile uint32_t*)lfb;
    uint32_t* src = back_buffer;
    for (uint32_t y = 0; y < screen_height; y++) {
        volatile uint32_t* line_dest = (volatile uint32_t*)((uint8_t*)dest + (y * screen_pitch));
        for (uint32_t x = 0; x < screen_width; x++) {
            line_dest[x] = src[y * screen_width + x];
        }
    }
}

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) {
            put_pixel(x + j, y + i, color);
        }
    }
}

void draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    unsigned char uc = (unsigned char)c;
    if (uc > 127) return; 
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 8; j++) {
            if (font_bitmap[uc][i] & (0x80 >> j)) {
                put_pixel(x + j, y + i, color);
            }
        }
    }
}

void draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color) {
    while (*str) {
        draw_char(*str++, x, y, color);
        x += 8;
    }
}

void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic;
    multiboot_info_t* mb = (multiboot_info_t*)addr;
    if (!(mb->flags & (1 << 12))) return;

    lfb           = (uint32_t*)mb->framebuffer_addr_low;
    screen_width  = mb->framebuffer_width;
    screen_height = mb->framebuffer_height;
    screen_pitch  = mb->framebuffer_pitch;
    back_buffer   = (uint32_t*)0x800000; 

    draw_rect(0, 0, screen_width, screen_height, 0x1d1a21); 

    draw_rect(0, 0, screen_width, 24, 0x2b2730);
    draw_string("Liege Kernel v0.1.0-alpha", 10, 4, 0xAAAAAA);
    draw_string("tty0", screen_width - 45, 4, 0x555555);

    draw_string("[  OK  ] Initializing VESA LFB", 15, 45, 0x00FF00);
    draw_string("[  OK  ] GDT/IDT descriptors loaded", 15, 65, 0x00FF00);
    draw_string("[  OK  ] Page directory initialized", 15, 85, 0x00FF00);

    draw_string("Display Info:", 15, 120, 0x00A8FF);
    draw_string("Resolution :", 30, 140, 0x777777);
    draw_string("1024x768 x 32bpp", 140, 140, 0xFFFFFF);

    draw_string("Framebuf   :", 30, 160, 0x777777);
    draw_string("Physical 0xFD000000", 140, 160, 0xFFFFFF);

    draw_string("Memory Map :", 15, 190, 0x00A8FF);
    draw_string("Upper Mem  :", 30, 210, 0x777777);
    draw_string("65536 KB", 140, 210, 0xFFFFFF);

    /* Push everything from RAM to Video Memory */
    graphics_swap_buffers();

    while (1) {
        __asm__ volatile ("hlt");
    }
}
