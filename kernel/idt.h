/* =============================================================================
 * SENG21213-OS :: IDT (Interrupt Descriptor Table)
 * File   : kernel/idt.h
 * ============================================================================*/
#ifndef IDT_H
#define IDT_H

#include "../include/types.h"

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);

#endif /* IDT_H */
