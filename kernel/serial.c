/* =============================================================================
 * SENG21213-OS :: COM1 Serial Port Driver (for -nographic / terminal testing)
 * File   : kernel/serial.c
 * Purpose: Mirrors kernel text output to COM1, which QEMU's -nographic mode
 *          redirects straight into the launching terminal. Also polls COM1
 *          for input, so the shell can be driven reliably from a terminal
 *          instead of a graphical QEMU window (WSLg's window focus/grab has
 *          been unreliable). VGA + PS/2 keyboard remain the primary I/O.
 * ============================================================================*/
#include "serial.h"
#include "io.h"
#include "../include/types.h"

#define COM1_PORT           0x3F8

#define COM1_DATA           (COM1_PORT + 0)
#define COM1_INT_ENABLE     (COM1_PORT + 1)
#define COM1_FIFO_CTRL      (COM1_PORT + 2)
#define COM1_LINE_CTRL      (COM1_PORT + 3)
#define COM1_MODEM_CTRL     (COM1_PORT + 4)
#define COM1_LINE_STATUS    (COM1_PORT + 5)

#define COM1_LINE_STATUS_THR_EMPTY 0x20
#define COM1_LINE_STATUS_DATA_READY 0x01

void serial_init(void) {
    outb(COM1_INT_ENABLE, 0x00);   /* disable all interrupts */
    outb(COM1_LINE_CTRL,  0x80);   /* enable DLAB (set baud rate divisor) */
    outb(COM1_DATA,       0x03);   /* divisor low byte  -> 38400 baud */
    outb(COM1_INT_ENABLE, 0x00);   /* divisor high byte */
    outb(COM1_LINE_CTRL,  0x03);   /* 8 bits, no parity, one stop bit */
    outb(COM1_FIFO_CTRL,  0xC7);   /* enable FIFO, clear, 14-byte threshold */
    outb(COM1_MODEM_CTRL, 0x0B);   /* IRQs disabled, RTS/DSR set */
}

static int serial_tx_empty(void) {
    return inb(COM1_LINE_STATUS) & COM1_LINE_STATUS_THR_EMPTY;
}

void serial_putchar(char c) {
    while (!serial_tx_empty());
    outb(COM1_DATA, (uint8_t)c);
}

int serial_has_char(void) {
    return inb(COM1_LINE_STATUS) & COM1_LINE_STATUS_DATA_READY;
}

char serial_read_char(void) {
    return (char)inb(COM1_DATA);
}
