#ifndef IDT_H
#define IDT_H

#include "stdint.h" 

/* one entry in the IDT — describes one interrupt handler */
/* packed tells the compiler not to add padding between fields */
struct idt_entry {
    uint16_t offset_low;   /* lower 16 bits of handler address */
    uint16_t selector;     /* code segment selector — always 0x08 for kernel */
    uint8_t  zero;         /* always zero */
    uint8_t  type_attr;    /* type and attributes — 0x8E for interrupt gate */
    uint16_t offset_high;  /* upper 16 bits of handler address */
} __attribute__((packed));

/* tells the CPU where the IDT is and how big it is */
struct idt_ptr {
    uint16_t limit;  /* size of IDT in bytes minus 1 */
    uint32_t base;   /* address of the IDT in memory */
} __attribute__((packed));

/* sets up the IDT and loads it into the CPU */
void idt_init(void);

/* registers one interrupt handler in the IDT */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);

#endif
