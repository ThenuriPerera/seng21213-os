/* =============================================================================
 * SENG21213-OS :: Mutex (spin-lock based)
 * File   : kernel/mutex.c
 * ============================================================================*/
#include "mutex.h"

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->wait_head = NULL;
}

void mutex_lock(mutex_t *m) {
    while (1) {
        __asm__ __volatile__("cli");
        if (!m->locked) {
            m->locked = 1;
            __asm__ __volatile__("sti");
            return;
        }
        __asm__ __volatile__("sti");
        thread_yield();
    }
}

void mutex_unlock(mutex_t *m) {
    __asm__ __volatile__("cli");
    m->locked = 0;
    __asm__ __volatile__("sti");
}
