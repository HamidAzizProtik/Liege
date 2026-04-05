#include "cpu/idt.h"
#include "drivers/keyboard.h"

/* defined in isr.asm — the raw interrupt entry point */
extern void irq1_handler(void);

/* reads one byte from a hardware I/O port */
/* port I/O is how x86 talks to hardware devices */
static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* writes one byte to a hardware I/O port */
static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

#define COM1 0x3F8

static void serial_init(void) {
    outb(COM1 + 1, 0x00); /* Disable interrupts */
    outb(COM1 + 3, 0x80); /* Enable DLAB */
    outb(COM1 + 0, 0x03); /* 38400 baud */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); /* 8 bits, no parity, one stop bit */
    outb(COM1 + 2, 0xC7); /* Enable FIFO */
    outb(COM1 + 4, 0x0B); /* RTS/DSR set */
}

static void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0);
    outb(COM1, c);
}

static void serial_puts(const char *s) {
    while (*s) serial_putc(*s++);
}

/* scancode to character lookup table */
/* index is the scancode, value is the character */
/* 0 means no printable character for that scancode */
static char scancode_table[128] = {
    0,   0,  '1', '2', '3', '4', '5', '6', '7', '8',  /* 0x00 - 0x09 */
   '9', '0', '-', '=', '\b', 0,  'q', 'w', 'e', 'r',  /* 0x0A - 0x13 */
   't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,   /* 0x14 - 0x1D */
   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  /* 0x1E - 0x27 */
   '\'', '`',  0, '\\','z', 'x', 'c', 'v', 'b', 'n',   /* 0x28 - 0x31 */
   'm', ',', '.', '/',  0,   0,   0,  ' ',  0,   0,   /* 0x32 - 0x3B */
};

/* scancode to uppercase character lookup table */
static char scancode_table_upper[128] = {
    0,   0,  '!', '@', '#', '$', '%', '^', '&', '*',  /* 0x00 - 0x09 */
   '(', ')', '_', '+', '\b', 0,  'Q', 'W', 'E', 'R',  /* 0x0A - 0x13 */
   'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,   /* 0x14 - 0x1D */
   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',  /* 0x1E - 0x27 */
   '"', '~',  0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',   /* 0x28 - 0x31 */
   'M', '<', '>', '?',  0,   0,   0,  ' ',  0,   0,   /* 0x32 - 0x3B */
};

/* stores the last character pressed */
static char last_key = 0;

/* keyboard state */
static int caps_lock = 0;  /* 0 = off, 1 = on */
static int shift_pressed = 0;  /* 0 = not pressed, 1 = pressed */

/* remaps the PIC so hardware interrupts use numbers 32-47 */
/* without this keyboard interrupt collides with CPU exceptions */
static void pic_remap(void) {
    outb(0x20, 0x11); /* master PIC — start initialization */
    outb(0xA0, 0x11); /* slave PIC — start initialization */

    outb(0x21, 0x20); /* master PIC starts at interrupt 32 */
    outb(0xA1, 0x28); /* slave PIC starts at interrupt 40 */

    outb(0x21, 0x04); /* master PIC — IRQ2 is slave */
    outb(0xA1, 0x02); /* slave PIC — cascade identity */

    outb(0x21, 0x01); /* 8086 mode */
    outb(0xA1, 0x01); /* 8086 mode */

    /* mask all interrupts except IRQ1 keyboard */
    /* 0xFD = 11111101 — bit 1 clear means IRQ1 unmasked */
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF); /* mask all slave PIC interrupts */
}

/* called by irq1_handler in isr.asm when a key is pressed */
void keyboard_handler(void) {
    /* read scancode from keyboard data port */
    unsigned char scancode = inb(0x60);

    /* handle key release events — bit 7 set means key was released */
    if (scancode & 0x80) {
        /* check if shift was released */
        unsigned char released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36) {  /* left or right shift */
            shift_pressed = 0;
        }
        return;
    }

    /* ignore scancodes outside our table */
    if (scancode >= 128) return;

    /* handle shift keys */
    if (scancode == 0x2A || scancode == 0x36) {  /* left or right shift */
        shift_pressed = 1;
        return;
    }

    /* handle caps lock toggle */
    if (scancode == 0x3A) {  /* caps lock */
        caps_lock = !caps_lock;
        return;
    }

    /* determine character based on shift and caps lock */
    char c = scancode_table[scancode];
    if (c >= 'a' && c <= 'z') {
        if (shift_pressed ^ caps_lock) c -= 32;  /* to uppercase */
    } else if (c >= 'A' && c <= 'Z') {
        if (shift_pressed ^ caps_lock) c += 32;  /* to lowercase, though unlikely */
    } else {
        if (shift_pressed) c = scancode_table_upper[scancode];
    }

    /* store it if it is a printable character */
    if (c != 0) {
        last_key = c;
    }
}

/* returns the last character typed and clears it */
char keyboard_getchar(void) {
    char c = last_key;
    last_key = 0; /* clear so we do not return the same key twice */
    return c;
}

/* wait for keyboard controller input buffer to be empty */
static void keyboard_wait_input(void) {
    while (inb(0x64) & 0x02);
}

/* wait for keyboard controller output buffer to be full */
static void keyboard_wait_output(void) {
    while (!(inb(0x64) & 0x01));
}

/* initializes the keyboard driver */
void keyboard_init(void) {
    /* initialize serial for debugging */
    serial_init();

    /* remap PIC first — must happen before registering handlers */
    pic_remap();

    /* initialize PS/2 keyboard controller */
    /* disable keyboard */
    keyboard_wait_input();
    outb(0x64, 0xAD);

    /* disable mouse (second port) */
    keyboard_wait_input();
    outb(0x64, 0xA7);

    /* flush output buffer */
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    /* read configuration byte */
    keyboard_wait_input();
    outb(0x64, 0x20);
    keyboard_wait_output();
    unsigned char config = inb(0x60);

    /* enable keyboard interrupt (bit 0) and enable translation (bit 6 = 1) */
    config |= 0x01;
    config |= 0x40;

    /* write back configuration */
    keyboard_wait_input();
    outb(0x64, 0x60);
    keyboard_wait_input();
    outb(0x60, config);

    /* enable keyboard */
    keyboard_wait_input();
    outb(0x64, 0xAE);

    /* register IRQ1 handler in the IDT at position 33 */
    /* 33 = 32 (PIC offset) + 1 (IRQ1 keyboard) */
    /* 0x08 = kernel code segment selector */
    /* 0x8E = interrupt gate, present, ring 0 */
    idt_set_gate(33, (uint32_t)irq1_handler, 0x10, 0x8E);
}
