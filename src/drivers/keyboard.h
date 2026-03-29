#ifndef KEYBOARD_H
#define KEYBOARD_H

/* initializes the keyboard — remaps PIC and registers IRQ1 handler */
void keyboard_init(void);

/* called by irq1_handler in isr.asm when a key is pressed */
void keyboard_handler(void);

/* returns the last character typed, 0 if no new key */
char keyboard_getchar(void);

#endif
