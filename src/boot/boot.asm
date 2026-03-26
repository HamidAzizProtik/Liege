; Multiboot header — tells GRUB this is a kernel
MAGIC    equ 0x1BADB002
FLAGS    equ 0
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Set up a small stack and jump to our C kernel
section .text
global _start

_start:
    mov esp, stack_top
    extern kernel_main
    call kernel_main
    hlt             ; halt if kernel returns

section .bss
align 16
stack_bottom:
    resb 16384      ; 16 KB stack
stack_top: