; isr.asm — interrupt service routines and IDT loader

[bits 32]
[global idt_load]
[global irq1_handler]
[extern keyboard_handler]

; loads the IDT pointer into the CPU
; called from idt_init() in idt.c
idt_load:
    mov eax, [esp + 4]  ; get pointer to idt_ptr struct
    lidt [eax]          ; load IDT into CPU
    ret

; IRQ1 handler — fires when a key is pressed
irq1_handler:
    pusha               ; save all registers

    call keyboard_handler  ; call C keyboard handler

    mov al, 0x20        ; EOI command
    out 0x20, al        ; send to master PIC

    popa                ; restore all registers
    iret                ; return from interrupt