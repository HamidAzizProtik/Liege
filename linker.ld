ENTRY(_start)

SECTIONS {
    . = 1M;               /* load kernel at 1MB — standard for x86 */

    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) : { *(.rodata) }
    .data   ALIGN(4K) : { *(.data)   }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }
}