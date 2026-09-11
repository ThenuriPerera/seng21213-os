/* =============================================================================
 * SENG21213-OS :: 8259 PIC (Programmable Interrupt Controller)
 * File   : kernel/pic.h
 * Purpose: Remap IRQs 0-15 to interrupt vectors 32-47, and provide EOI.
 * ============================================================================*/
#ifndef PIC_H
#define PIC_H

#include "../include/types.h"

void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);

#endif /* PIC_H */
