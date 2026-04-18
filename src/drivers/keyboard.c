#include "cpu/idt.h"
#include "drivers/keyboard.h"

/* defined in isr.asm — the raw interrupt entry point */
extern void irq1_handler(void);

/* reads one byte from a hardware I/O port */
static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* writes one byte to a hardware I/O port */
static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* scancode to character lookup table */
static char scancode_table[128] = {
    0,   0,  '1', '2', '3', '4', '5', '6', '7', '8',
   '9', '0', '-', '=', '\b', 0,  'q', 'w', 'e', 'r',
   't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
   '\'', '`',  0, '\\','z', 'x', 'c', 'v', 'b', 'n',
   'm', ',', '.', '/',  0,   0,   0,  ' ',  0,   0,
};

/* scancode to uppercase character lookup table */
static char scancode_table_upper[128] = {
    0,   0,  '!', '@', '#', '$', '%', '^', '&', '*',
   '(', ')', '_', '+', '\b', 0,  'Q', 'W', 'E', 'R',
   'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
   '"', '~',  0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
   'M', '<', '>', '?',  0,   0,   0,  ' ',  0,   0,
};

static char last_key    = 0;
static int  caps_lock   = 0;
static int  shift_pressed = 0;

/* remaps PIC so hardware interrupts use numbers 32-47 */
static void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFD); /* unmask IRQ1 only */
    outb(0xA1, 0xFF); /* mask all slave PIC interrupts */
}

/* called by irq1_handler when a key is pressed */
void keyboard_handler(void) {
    unsigned char scancode = inb(0x60);

    /* handle key release */
    if (scancode & 0x80) {
        unsigned char released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36)
            shift_pressed = 0;
        return;
    }

    if (scancode >= 128) return;

    /* shift keys */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    /* caps lock */
    if (scancode == 0x3A) {
        caps_lock = !caps_lock;
        return;
    }

    char c = scancode_table[scancode];
    if (c >= 'a' && c <= 'z') {
        if (shift_pressed ^ caps_lock) c -= 32;
    } else if (c >= 'A' && c <= 'Z') {
        if (shift_pressed ^ caps_lock) c += 32;
    } else {
        if (shift_pressed) c = scancode_table_upper[scancode];
    }

    if (c != 0) last_key = c;
}

/* returns last character typed and clears it */
char keyboard_getchar(void) {
    char c = last_key;
    last_key = 0;
    return c;
}

/* waits for keyboard controller input buffer to empty */
static void keyboard_wait_input(void) {
    while (inb(0x64) & 0x02);
}

/* waits for keyboard controller output buffer to fill */
static void keyboard_wait_output(void) {
    while (!(inb(0x64) & 0x01));
}

/* initializes the keyboard driver */
void keyboard_init(void) {
    pic_remap();

    keyboard_wait_input();
    outb(0x64, 0xAD); /* disable keyboard */

    keyboard_wait_input();
    outb(0x64, 0xA7); /* disable mouse */

    /* flush output buffer */
    while (inb(0x64) & 0x01) inb(0x60);

    /* read and update configuration byte */
    keyboard_wait_input();
    outb(0x64, 0x20);
    keyboard_wait_output();
    unsigned char config = inb(0x60);
    config |= 0x01; /* enable keyboard interrupt */
    config |= 0x40; /* enable translation */
    keyboard_wait_input();
    outb(0x64, 0x60);
    keyboard_wait_input();
    outb(0x60, config);

    keyboard_wait_input();
    outb(0x64, 0xAE); /* enable keyboard */

    /* register IRQ1 handler at IDT slot 33 */
    idt_set_gate(33, (uint32_t)irq1_handler, 0x10, 0x8E);
}