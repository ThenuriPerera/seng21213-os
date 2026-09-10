/* =============================================================================
 * SENG21213-OS :: Low-level port I/O helpers
 * File   : kernel/io.h
 * ============================================================================*/
#ifndef IO_H
#define IO_H

#include "../include/types.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    /* Small delay, needed after some PIC/PIT writes on real hardware.
       Writing to an unused port (0x80) burns a few cycles. */
    __asm__ __volatile__("outb %%al, $0x80" : : "a"(0));
}

#endif /* IO_H */
