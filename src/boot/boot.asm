; Multiboot header — tells GRUB this is a kernel
MAGIC    equ 0x1BADB002               ; definining special number grub looks for (equ = define)
FLAGS    equ 0                        ; 0 flag means simple aka no special features
CHECKSUM equ -(MAGIC + FLAGS)         ; makes sure that magic + flags + checksum = 0 (checking)

; puts the header within first 8 kilobytes of kernel for easier access
section .multiboot                    ; data for grub
; align 4 so that cpu address is divisible by 4 (organised)
align 4 
    dd MAGIC                          ; allocates 4 bytes 
    dd FLAGS                          ; [MAGIC][FLAGS][CHECKSUM] this is bootable
    dd CHECKSUM                       ; basically a kernel's ID card

; Set up a small stack and jump to our C kernel
section .text                         ; actual instructions
global _start                         ; exposes start so that kernel knows its the entry point

; main entry point of the kernel
_start:
    mov esp, stack_top

    push ebx        ; 2nd argument (addr)
    push eax        ; 1st argument (magic)

    extern kernel_main
    call kernel_main

    hlt                              ; halt if kernel returns

; code for stack
section .bss                          ; sections off memory to make a stack required for C
align 16                              ; address is divisible by 16
stack_bottom:                         ; (stack grows downwards)
    resb 16384                        ; reserves a 16 KB stack
stack_top: