/* Write directly to VGA text buffer — no printf, no stdlib */
#define VGA_WIDTH  80 /* a preprocessor directive telling the computer that vga height and width is 80 and 25 respectively*/
#define VGA_HEIGHT 25

/* a pointer called vga that lets the program write directly to the vga buffer */
static unsigned short *vga = (unsigned short *)0xB8000;

/* global cursor position — print reads and updates these automatically */
static int cursor_row = 0;
static int cursor_col = 0;

/* creating a function that clears the screen */
static void clear(void) {
    /* for loop iterating through the whole buffer clearing everything */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (unsigned short)(' ' | (0x07 << 8)); /* clears everything */
    /* reset cursor to top left after clearing */
    cursor_row = 0;
    cursor_col = 0;
}

/* prints a single character and advances the cursor */
static void print_char(char c) {
    /* handle newline — move to next row, reset to left edge */
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        return; /* nothing to draw, just move cursor */
    }

    /* calculate position in the flat VGA array from row and col */
    /* row * 80 skips to the right row, + cursor_col gets the exact cell */
    int index = cursor_row * VGA_WIDTH + cursor_col;
    vga[index] = (unsigned short)(c | (0x0F << 8)); /* white on black */

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

/* print a string but centered */
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

/* actual kernel code */
void kernel_main(void) {
    clear(); /* clearing screen before operation */

    print_center("Liege OS\n"); /* \n moves cursor to next line automatically */
    print_center("Hello from Liege!\n");
    print("test\n");
    print("liege> ");

    while (1) {}  /* spin forever */
}