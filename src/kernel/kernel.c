#include "stdint.h"
#include "memory/multiboot.h"
#include "memory/pmm.h"
#include "cpu/idt.h"
#include "drivers/keyboard.h"
#include "fonts.h"

#define CHAR_W        8
#define CHAR_H        16
#define TERM_MARGIN   15
#define TITLEBAR_H    28
#define BG_COLOR      0x1d1a21
#define TITLE_BG      0x2b2730

static uint32_t* lfb;
static uint32_t* back_buffer;
static uint32_t  screen_width;
static uint32_t  screen_height;
static uint32_t  screen_pitch;

static inline void put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x < screen_width && y < screen_height)
        back_buffer[y * screen_width + x] = color;
}

static void graphics_swap_buffers(void) {
    for (uint32_t y = 0; y < screen_height; y++) {
        volatile uint32_t* line = (volatile uint32_t*)((uint8_t*)lfb + (y * screen_pitch));
        for (uint32_t x = 0; x < screen_width; x++)
            line[x] = back_buffer[y * screen_width + x];
    }
}

static void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t i = 0; i < h; i++)
        for (uint32_t j = 0; j < w; j++)
            put_pixel(x + j, y + i, color);
}

static void draw_char(char c, uint32_t x, uint32_t y, uint32_t color) {
    unsigned char uc = (unsigned char)c;
    if (uc > 127) return;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 8; j++)
            if (font_bitmap[uc][i] & (0x80 >> j))
                put_pixel(x + j, y + i, color);
}

static void draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color) {
    while (*str) { draw_char(*str++, x, y, color); x += 8; }
}

static int graphics_init_backbuffer(void) {
    uint32_t pages = (screen_width * screen_height * 4 + 4095) / 4096;
    void* first = pmm_alloc_page();
    if (!first) return 0;
    for (uint32_t i = 1; i < pages; i++)
        if (!pmm_alloc_page()) return 0;
    back_buffer = (uint32_t*)first;
    return 1;
}

static uint32_t term_x;
static uint32_t term_y;
static uint32_t term_color = 0xFFFFFF;
static uint32_t term_top;

static void term_scroll(void) {
    uint32_t line_bytes = screen_width;
    for (uint32_t y = term_top + CHAR_H; y < screen_height; y++)
        for (uint32_t x = 0; x < screen_width; x++)
            back_buffer[(y - CHAR_H) * line_bytes + x] = back_buffer[y * line_bytes + x];
    draw_rect(0, screen_height - CHAR_H, screen_width, CHAR_H, BG_COLOR);
    term_y = screen_height - CHAR_H;
}

static void term_putchar(char c) {
    if (c == '\n') {
        term_x  = TERM_MARGIN;
        term_y += CHAR_H;
        if (term_y + CHAR_H > screen_height) term_scroll();
        return;
    }
    if (c == '\b') {
        if (term_x > TERM_MARGIN) {
            term_x -= CHAR_W;
            draw_rect(term_x, term_y, CHAR_W, CHAR_H, BG_COLOR);
        }
        return;
    }
    draw_char(c, term_x, term_y, term_color);
    term_x += CHAR_W;
    if (term_x + CHAR_W > screen_width - TERM_MARGIN) {
        term_x  = TERM_MARGIN;
        term_y += CHAR_H;
        if (term_y + CHAR_H > screen_height) term_scroll();
    }
}

static void term_print(const char* str) {
    while (*str) term_putchar(*str++);
}

static void term_println(const char* str) {
    term_print(str);
    term_putchar('\n');
}

static void term_print_uint(uint32_t n) {
    char buf[11]; buf[10] = '\0'; int i = 9;
    if (n == 0) { term_putchar('0'); return; }
    while (n > 0) { buf[i--] = '0' + (n % 10); n /= 10; }
    term_print(&buf[i + 1]);
}

static void term_print_hex(uint32_t n) {
    char d[] = "0123456789ABCDEF";
    char buf[9]; buf[8] = '\0';
    for (int i = 7; i >= 0; i--) { buf[i] = d[n & 0xF]; n >>= 4; }
    term_print("0x");
    term_print(buf);
}

static void term_print_prompt(void) {
    term_color = 0x9d7cd8;
    term_print("liege");
    term_color = 0x555555;
    term_print(" > ");
    term_color = 0xFFFFFF;
}

static int str_eq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) { if (a[i] != b[i]) return 0; i++; }
    return a[i] == b[i];
}

static int str_len(const char* s) {
    int i = 0; while (s[i]) i++; return i;
}

static const char* str_after_space(const char* s) {
    while (*s && *s != ' ') s++;
    while (*s == ' ') s++;
    return s;
}

static multiboot_info_t* g_mb;

static void shell_execute(const char* cmd) {
    if (str_len(cmd) == 0) return;
    if (str_eq(cmd, "help")) {
        term_color = 0x00A8FF;
        term_println("commands:");
        term_color = 0xFFFFFF;
        term_println("  help   clear   about   mem   echo");
    } else if (str_eq(cmd, "clear")) {
        draw_rect(0, term_top, screen_width, screen_height - term_top, BG_COLOR);
        term_x = TERM_MARGIN;
        term_y = term_top;
    } else if (str_eq(cmd, "about")) {
        term_color = 0x9d7cd8;
        term_println("Liege OS v0.1.0-alpha");
        term_color = 0x777777;
        term_println("minimal x86 operating system");
    } else if (str_eq(cmd, "mem")) {
        term_color = 0x00A8FF;
        term_print("usage: ");
        term_color = 0xFFFFFF;
        term_print_uint(pmm_get_used() * 4);
        term_print(" / ");
        term_print_uint(pmm_get_total() * 4);
        term_println(" KB");
    } else if (str_len(cmd) >= 4 && cmd[0]=='e' && cmd[1]=='c' && cmd[2]=='h' && cmd[3]=='o') {
        term_println(str_after_space(cmd));
    } else {
        term_color = 0xcc6666;
        term_print("error: unknown command '");
        term_print(cmd);
        term_println("'");
    }
}

void kernel_main(uint32_t magic, uint32_t addr) {
    idt_init();
    keyboard_init();
    __asm__ volatile ("sti");

    if (magic != 0x2BADB002) while (1) __asm__ volatile ("hlt");

    multiboot_info_t* mb = (multiboot_info_t*)addr;
    g_mb = mb;

    lfb           = (uint32_t*)mb->framebuffer_addr_low;
    screen_width  = mb->framebuffer_width;
    screen_height = mb->framebuffer_height;
    screen_pitch  = mb->framebuffer_pitch;

    pmm_init(mb);
    graphics_init_backbuffer();

    draw_rect(0, 0, screen_width, screen_height, BG_COLOR);
    draw_rect(0, 0, screen_width, TITLEBAR_H, TITLE_BG);
    draw_string("Liege OS v0.1.0-alpha", 10, 6, 0xAAAAAA);
    draw_string("tty0", screen_width - 45, 6, 0x444444);
    draw_rect(0, TITLEBAR_H, screen_width, 1, 0x3d3a45);

    term_top = TITLEBAR_H + 1;
    term_x = TERM_MARGIN;
    term_y = term_top + 16;

    term_color = 0x00cc66;
    term_println("OK   IDT initialized");
    term_println("OK   Keyboard ready");
    term_println("OK   PMM initialized");
    term_println("OK   Back buffer allocated");
    term_putchar('\n');

    term_color = 0x00A8FF;
    term_println("display");
    term_color = 0x555555;
    term_print("  width  : "); term_color = 0xFFFFFF; term_print_uint(screen_width); term_putchar('\n');
    term_color = 0x555555;
    term_print("  height : "); term_color = 0xFFFFFF; term_print_uint(screen_height); term_putchar('\n');
    term_color = 0x555555;
    term_print("  addr   : "); term_color = 0xFFFFFF; term_print_hex(mb->framebuffer_addr_low); term_putchar('\n');
    term_putchar('\n');

    term_color = 0x00A8FF;
    term_println("memory");
    term_color = 0x555555;
    term_print("  upper  : "); term_color = 0xFFFFFF; term_print_uint(mb->mem_upper); term_println(" KB");
    term_putchar('\n');

    term_color = 0x555555;
    term_println("type 'help' for commands");
    term_putchar('\n');
    term_print_prompt();
    graphics_swap_buffers();

    char input[256];
    int input_len = 0;
    int cursor_visible = 1;
    uint32_t blink_counter = 0;

    while (1) {
        blink_counter++;
        if (blink_counter >= 800000) {
            blink_counter = 0;
            cursor_visible = !cursor_visible;
            draw_rect(term_x, term_y + CHAR_H - 2, CHAR_W, 2, cursor_visible ? 0x9d7cd8 : BG_COLOR);
            graphics_swap_buffers();
        }

        char c = keyboard_getchar();
        if (c == 0) continue;

        draw_rect(term_x, term_y + CHAR_H - 2, CHAR_W, 2, BG_COLOR);
        if (c == '\n') {
            term_putchar('\n');
            input[input_len] = '\0';
            term_color = 0xFFFFFF;
            shell_execute(input);
            input_len = 0;
            term_putchar('\n');
            term_print_prompt();
        } else if (c == '\b' && input_len > 0) {
            input_len--;
            term_putchar('\b');
        } else if (c != '\b' && c != '\n' && input_len < 255) {
            input[input_len++] = c;
            term_putchar(c);
        }
        graphics_swap_buffers();
    }
}