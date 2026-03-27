/* Write directly to VGA text buffer — no printf, no stdlib */
#define VGA_WIDTH  80 /* a preprocessor directive telling the computer that vga height and width is 80 and 25 respectively*/
#define VGA_HEIGHT 25

/* a pointer called vgs that lets the program write directly to the vgs buffer */
static unsigned short *vga = (unsigned short *)0xB8000;

/* creating a function that clears the screen */
static void clear_screen(void) {
    /* for loop iterating through the whole buffer clearing everything */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (unsigned short)(' ' | (0x07 << 8)); /* clears everything*/
}

/* creating a basic print statement function (will change later on) */
static void print(const char *str, int row, int col) {
    int i = 0; /* initializing intger for loop */
    while (str[i]) { /* loop till end of string */
        vga[row * VGA_WIDTH + col + i] = /* calculates where to print */
            (unsigned short)(str[i] | (0x0F << 8));  /* white on black (white text shifting to upper byte) */
        i++; /* this bitwise or and upper byte operation makes the vga buffer print*/
    }
}

/* actual kernel code */
void kernel_main(void) {
    clear_screen(); /* clearing screen before operation */

    print("Hello from Liege!", 2, 31); /* print statements */
    print("test", 4, 2);

    while (1) {}  /* spin forever */
}