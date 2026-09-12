/* =============================================================================
 * SENG21213-OS :: COM1 Serial Port Driver (for -nographic / terminal testing)
 * File   : kernel/serial.h
 * ============================================================================*/
#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);
void serial_putchar(char c);
int  serial_has_char(void);
char serial_read_char(void);

#endif /* SERIAL_H */
