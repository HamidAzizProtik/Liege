#include "cpu/idt.h"

/* the actual IDT — 256 entries, one per possible interrupt */
static struct idt_entry idt[256];

/* pointer structure that we give to the CPU via lidt instruction */
static struct idt_ptr idtp;

/* defined in isr.asm — loads the IDT pointer into the CPU */
extern void idt_load(struct idt_ptr *ptr);

/* registers one handler in the IDT at position num */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = handler & 0xFFFF;         /* lower 16 bits of address */
    idt[num].selector    = selector;                  /* kernel code segment */
    idt[num].zero        = 0;                         /* always zero */
    idt[num].type_attr   = flags;                     /* interrupt gate flags */
    idt[num].offset_high = (handler >> 16) & 0xFFFF; /* upper 16 bits of address */
}

/* initializes the IDT and loads it into the CPU */
void idt_init(void) {
    /* tell the CPU where our IDT lives and how big it is */
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    /* zero out all 256 entries with valid selector but not present */
    /* type_attr = 0 means not present — CPU will ignore these entries */
    /* selector = 0x08 keeps a valid segment so no GPF on lookup */
    for (int i = 0; i < 256; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0x08; /* valid kernel code segment — not 0 */
        idt[i].zero        = 0;
        idt[i].type_attr   = 0;    /* not present — CPU will ignore */
        idt[i].offset_high = 0;
    }

    /* load the IDT — defined in isr.asm */
    idt_load(&idtp);
}