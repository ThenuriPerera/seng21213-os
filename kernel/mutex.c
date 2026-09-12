/* =============================================================================
 * SENG21213-OS :: Mutex (blocking, with a FIFO wait list)
 * File   : kernel/mutex.c
 * Purpose: mutex_lock() blocks the calling thread (state = BLOCKED, removed
 *          from the scheduler's ready rotation via thread_block()) if the
 *          lock is already held, instead of busy-waiting. mutex_unlock()
 *          hands the lock directly to the next waiter (if any) and wakes it;
 *          if no one is waiting, the lock is simply released.
 * ============================================================================*/
#include "mutex.h"

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->wait_head = NULL;
}

void mutex_lock(mutex_t *m) {
    __asm__ __volatile__("cli");

    if (!m->locked) {
        m->locked = 1;
        __asm__ __volatile__("sti");
        return;
    }

    /* Lock is held: enqueue ourselves at the tail of the FIFO wait list */
    thread_t *self = thread_current();
    self->next_wait = NULL;

    if (m->wait_head == NULL) {
        m->wait_head = self;
    } else {
        thread_t *tail = m->wait_head;
        while (tail->next_wait != NULL) {
            tail = tail->next_wait;
        }
        tail->next_wait = self;
    }

    __asm__ __volatile__("sti");

    /* Block until mutex_unlock() hands us the lock and wakes us up */
    thread_block();
}

void mutex_unlock(mutex_t *m) {
    __asm__ __volatile__("cli");

    if (m->wait_head != NULL) {
        /* Hand the lock directly to the first waiter -- avoids a race
           where some other thread could steal the lock between us
           clearing it and the woken thread actually running. */
        thread_t *next_owner = m->wait_head;
        m->wait_head = next_owner->next_wait;
        next_owner->next_wait = NULL;
        /* m->locked stays 1 -- ownership transfers, not released */
        thread_unblock(next_owner);
    } else {
        m->locked = 0;
    }

    __asm__ __volatile__("sti");
}
