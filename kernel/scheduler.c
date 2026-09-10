/* =============================================================================
 * SENG21213-OS :: Scheduler / PIT (i8253 timer)
 * File   : kernel/scheduler.c
 * Purpose: Programs the PIT for ~100 Hz, installs the IRQ0 handler, and
 *          calls scheduler_tick() on every timer interrupt.
 * ============================================================================*/
#include "scheduler.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include "../include/types.h"
#include "../include/process.h"

#define PIT_CHANNEL0  0x40
#define PIT_COMMAND   0x43
#define PIT_FREQUENCY 1193182
#define TARGET_HZ     100

#define IDT_IRQ0_VECTOR 32   /* IRQ0 lands here after PIC remap (base 0x20) */
#define IDT_FLAG_INT32  0x8E /* present, ring 0, 32-bit interrupt gate */
#define KERNEL_CS       0x08 /* code segment selector — confirm against your GDT */

static volatile uint32_t tick_count = 0;

/* Defined in boot/isr_stub.asm */
extern void irq0_stub(void);

/* Called from irq0_stub (assembly) on every timer interrupt */
void irq0_handler(void) {
    tick_count++;
    pic_send_eoi(0);
    scheduler_tick();
}

static void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_FREQUENCY / hz;

    outb(PIT_COMMAND, 0x36); /* channel 0, lobyte/hibyte, mode 3 (square wave) */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

void scheduler_init(void) {
    idt_set_gate(IDT_IRQ0_VECTOR, (uint32_t)irq0_stub, KERNEL_CS, IDT_FLAG_INT32);
    pit_init(TARGET_HZ);
}
