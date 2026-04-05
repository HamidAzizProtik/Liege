#include "cpu/idt.h"
#include "drivers/keyboard.h"
#include "memory/multiboot.h"
#include "memory/pmm.h"

/* I/O port functions */
static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* raw interrupt entry point defined in isr.asm */
extern void irq1_handler(void);

/* Write directly to VGA text buffer — no printf, no stdlib */
#define VGA_WIDTH  80 /* a preprocessor directive telling the computer that vga width and height is 80 and 25 respectively */
#define VGA_HEIGHT 25

/* VGA foreground color codes */
#define COLOR_BLACK        0x0
#define COLOR_BLUE         0x1
#define COLOR_GREEN        0x2
#define COLOR_CYAN         0x3
#define COLOR_RED          0x4
#define COLOR_MAGENTA      0x5
#define COLOR_BROWN        0x6
#define COLOR_WHITE        0x7
#define COLOR_GRAY         0x8
#define COLOR_LIGHT_BLUE   0x9
#define COLOR_LIGHT_GREEN  0xA
#define COLOR_LIGHT_CYAN   0xB
#define COLOR_LIGHT_RED    0xC
#define COLOR_LIGHT_YELLOW 0xE
#define COLOR_BRIGHT_WHITE 0xF

/* a pointer called vga that lets the program write directly to the vga buffer */
static unsigned short *vga = (unsigned short *)0xB8000;

/* global cursor position — print reads and updates these automatically */
static int cursor_row = 0;
static int cursor_col = 0;

/* current color — default bright white on black */
static unsigned char current_color = COLOR_BRIGHT_WHITE;

/* sets the current text color — all prints after this use the new color */
static void set_color(unsigned char color) {
    current_color = color;
}

/* clears the screen and resets cursor to top left */
static void clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (unsigned short)(' ' | (0x07 << 8)); /* clears everything */
    cursor_row = 0;
    cursor_col = 0;
}

/* scroll screen up by one line */
static void scroll(void) {
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga[(row - 1) * VGA_WIDTH + col] =
                vga[row * VGA_WIDTH + col];
        }
    }

    /* clear last line */
    for (int col = 0; col < VGA_WIDTH; col++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            (unsigned short)(' ' | (current_color << 8));
    }

    cursor_row = VGA_HEIGHT - 1;
}

/* string compare for shell */
int strcmp(const char *a, const char *b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}

/* prints a single character and advances the cursor */
static void print_char(char c) {
    /* handle newline — move to next row, reset to left edge */
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;

        if (cursor_row >= VGA_HEIGHT) {
            scroll();
        }

        return;
    }

    /* handle backspace — move cursor back and clear the character */
    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            int index = cursor_row * VGA_WIDTH + cursor_col;
            vga[index] = (unsigned short)(' ' | (current_color << 8)); /* clear with space */
        } else if (cursor_row > 0) {
            /* move to end of previous line */
            cursor_row--;
            cursor_col = VGA_WIDTH - 1;
            int index = cursor_row * VGA_WIDTH + cursor_col;
            vga[index] = (unsigned short)(' ' | (current_color << 8)); /* clear with space */
        }
        return;
    }

    /* calculate position in the flat VGA array from row and col */
    /* row * 80 skips to the right row, + cursor_col gets the exact cell */
    int index = cursor_row * VGA_WIDTH + cursor_col;
    vga[index] = (unsigned short)(c | (current_color << 8)); /* uses current color */

    /* advance cursor forward by one */
    cursor_col++;

    /* if we hit the right edge, wrap to the next line */
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
}

/* print a string — no row/col needed, cursor tracks position automatically */
static void print(const char *str) {
    int i = 0;
    while (str[i]) { /* loop till end of string */
        print_char(str[i]);
        i++;
    }
}

/* print a string centered on the current row */
static void print_center(const char *str) {
    /* calculate string length */
    int len = 0;
    while (str[len]) len++;
    /* move cursor to center starting position */
    cursor_col = (VGA_WIDTH - len) / 2;

    int i = 0;
    while (str[i]) {
        print_char(str[i]);
        i++;
    }
}

/* prints a number in hexadecimal format e.g. 0xB8000 */
static void print_hex(unsigned int n) {
    char digits[] = "0123456789abcdef"; /* lookup table */

    /* buffer to hold 8 hex digits + null terminator */
    char buf[9];
    buf[8] = '\0'; /* null terminator — marks end of string */

    /* fill buffer from right to left */
    for (int i = 7; i >= 0; i--) {
        buf[i] = digits[n & 0xF]; /* extract lowest 4 bits, look up character */
        n >>= 4;                  /* shift right by 4 to get next digit */
    }

    print("0x"); /* prefix so it looks like 0xB8000 */
    print(buf);  /* print the 8 hex digits */
}

/* prints a decimal integer */
static void print_int(int n) {
    /* handle negative numbers */
    if (n < 0) {
        print_char('-');
        n = -n;
    }

    /* handle zero as a special case */
    if (n == 0) {
        print_char('0');
        return;
    }

    /* buffer to hold digits — 10 digits max for a 32 bit int */
    char buf[11];
    buf[10] = '\0'; /* null terminator */
    int i = 9;

    /* fill buffer right to left by extracting one digit at a time */
    while (n > 0) {
        buf[i] = '0' + (n % 10); /* remainder gives current digit */
        n /= 10;                  /* divide to move to next digit */
        i--;
    }

    print(&buf[i + 1]); /* print from first real digit */
}

/* kernel panic — something went catastrophically wrong, print and halt */
static void panic(const char *msg) {
    clear();
    set_color(COLOR_LIGHT_RED);
    print_center("KERNEL PANIC");
    print("\n\n");
    set_color(COLOR_BRIGHT_WHITE);
    print_center(msg);
    while (1) {} /* halt */
}

/* actual kernel code */
void kernel_main(unsigned int magic, unsigned int addr) {
    clear();

    /* initialize IDT and keyboard before anything else */
    idt_init();
    keyboard_init();

    /* enable interrupts — CPU will now respond to hardware signals */
    __asm__ volatile ("sti");

    /* banner in magenta */
    set_color(COLOR_MAGENTA);
    print("\n");
    print_center("L I E G E");

    /* separator in gray */
    set_color(COLOR_GRAY);
    print("\n");
    print_center("========================================");
    print("\n");

    /* version and tagline */
    set_color(COLOR_LIGHT_RED);
    print_center("v0.1  -  x86 32-bit  -  bare metal");
    print("\n");
    print_center("a minimal operating system");
    print("\n");
    set_color(COLOR_GRAY);
    print_center("========================================");
    print("\n\n");

    /* system status heading */
    set_color(COLOR_BRIGHT_WHITE);
    print("  SYSTEM\n");

    /* kernel loaded — always true if we are running */
    set_color(COLOR_LIGHT_GREEN);
    print("  [ OK ]");
    set_color(COLOR_BRIGHT_WHITE);
    print("  Kernel loaded\n");

    /* VGA initialized — always true if we can print */
    set_color(COLOR_LIGHT_GREEN);
    print("  [ OK ]");
    set_color(COLOR_BRIGHT_WHITE);
    print("  VGA initialized\n");

    /* keyboard — initialized and ready */
    set_color(COLOR_LIGHT_GREEN);
    print("  [ OK ]");
    set_color(COLOR_BRIGHT_WHITE);
    print("  Keyboard\n");

    /* filesystem — not implemented yet */
    set_color(COLOR_LIGHT_YELLOW);
    print("  [ -- ]");
    set_color(COLOR_GRAY);
    print("  Filesystem\n");

    /* memory map check if ready */

    multiboot_info_t* mb = (multiboot_info_t*) addr;

    if (!(mb->flags & (1 << 6))) {
        panic("No memory map!");
    }
    set_color(COLOR_BRIGHT_WHITE);
    print("\nMemory Map:\n");

    unsigned int mmap_addr = mb->mmap_addr;
    unsigned int mmap_end  = mmap_addr + mb->mmap_length;

    while (mmap_addr < mmap_end) {
        multiboot_memory_map_t* mmap =
            (multiboot_memory_map_t*) mmap_addr;

        print("Base: ");
        print_hex((unsigned int)mmap->addr);

        print("  Length: ");
        print_hex((unsigned int)mmap->len);

        print("  Type: ");
        print_int(mmap->type);

        print("\n");

        mmap_addr += mmap->size + sizeof(mmap->size);
    }

    /* separator */
    set_color(COLOR_GRAY);
    print("\n");
    print_center("========================================");
    print("\n\n");

    if (magic != 0x2BADB002) {
        panic("Invalid multiboot magic!");
    }

    pmm_init(mb);

    void* a = pmm_alloc_page();
    if (!a) panic("PMM out of memory");

    void* b = pmm_alloc_page();
    if (!b) panic("PMM out of memory");

    /* prompt in light blue */
    set_color(COLOR_LIGHT_BLUE);
    print("  / > ");
    set_color(COLOR_BRIGHT_WHITE); /* reset color after prompt */

    char input[128];
    int input_len = 0;

    /* main loop — simple shell */
    while (1) {
        char c = keyboard_getchar();

        if (c == 0) continue;

        if (c == '\n') {
            print_char('\n');

            input[input_len] = '\0';  /* terminate string */

            /* COMMAND HANDLER */
            if (strcmp(input, "hello")) {
                set_color(COLOR_LIGHT_GREEN);
                print("Hello from Liege kernel\n");
                set_color(COLOR_BRIGHT_WHITE);
            } else if (strcmp(input, "clear")) {
                clear();
            } else {
                print("Unknown command\n");
            }

            /* reset buffer */
            input_len = 0;

            /* prompt again */
            set_color(COLOR_LIGHT_BLUE);
            print("  / > ");
            set_color(COLOR_BRIGHT_WHITE);

        } else if (c == '\b') {
            if (input_len > 0) {
                input_len--;
                print_char('\b');
            }

        } else {
            if (input_len < 127) {
                input[input_len++] = c;
                print_char(c);
            }
        }
    }
}