/* =============================================================================
 * SENG21213-OS :: Scheduler / PIT (i8253 timer)
 * File   : kernel/scheduler.h
 * ============================================================================*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

void scheduler_init(void);   /* Programs the PIT, installs the IDT gate for IRQ0 */

#endif /* SCHEDULER_H */
