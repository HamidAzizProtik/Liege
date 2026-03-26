/* Write directly to VGA text buffer — no printf, no stdlib */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static unsigned short *vga = (unsigned short *)0xB8000;

static void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (unsigned short)(' ' | (0x07 << 8));
}

static void print(const char *str, int row, int col) {
    int i = 0;
    while (str[i]) {
        vga[row * VGA_WIDTH + col + i] =
            (unsigned short)(str[i] | (0x0F << 8));  /* white on black */
        i++;
    }
}

void kernel_main(void) {
    clear_screen();

    print("Hello from Liege!", 2, 31);
    print("test", 4, 2);

    while (1) {}  /* spin forever */
}