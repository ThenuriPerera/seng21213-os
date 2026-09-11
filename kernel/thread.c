/* =============================================================================
 * SENG21213-OS :: Kernel threads
 * File   : kernel/thread.c
 * ============================================================================*/
#include "thread.h"

extern void context_switch(uint32_t *old_sp, uint32_t new_sp);

static thread_t thread_table[MAX_THREADS];
static int      thread_count = 0;
static uint32_t next_tid     = 1;
static thread_t *ready_head  = NULL;
static thread_t *current     = NULL;
static uint32_t caller_esp   = 0;

void thread_init(void) {
    thread_count = 0;
    next_tid = 1;
    ready_head = NULL;
    current = NULL;
}

thread_t *thread_create(void (*entry)(void *arg), void *arg) {
    if (thread_count >= MAX_THREADS) {
        return NULL;
    }

    thread_t *t = &thread_table[thread_count];
    uint32_t *stack_top = &t->stack[STACK_SIZE / 4];

    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = (uint32_t)thread_exit;
    *(--stack_top) = (uint32_t)entry;

    *(--stack_top) = 0; *(--stack_top) = 0; *(--stack_top) = 0; *(--stack_top) = 0;
    *(--stack_top) = 0; *(--stack_top) = 0; *(--stack_top) = 0; *(--stack_top) = 0;

    t->tid   = next_tid++;
    t->state = READY;
    t->esp   = (uint32_t)stack_top;
    t->eip   = (uint32_t)entry;
    t->name[0] = '\0';
    t->next  = NULL;

    if (ready_head == NULL) {
        ready_head = t;
        t->next = t;
    } else {
        thread_t *tail = ready_head;
        while (tail->next != ready_head) tail = tail->next;
        tail->next = t;
        t->next = ready_head;
    }

    thread_count++;
    return t;
}

/* NOTE: timer protection during multi-threaded demos is handled at the PIC
 * level (pic_mask_irq(0) / pic_unmask_irq(0) in kernel.c), not by toggling
 * the CPU interrupt flag here. context_switch (boot/switch.asm) always
 * ends with `sti`, which is fine -- with IRQ0 masked at the PIC, `sti`
 * re-enabling the CPU flag doesn't let the timer through anyway. */
void thread_yield(void) {
    if (ready_head == NULL) {
        return;
    }

    if (current == NULL) {
        current = ready_head;
        current->state = RUNNING;
        context_switch(&caller_esp, current->esp);
        return;
    }

    thread_t *old = current;

    if (old->state == RUNNING) {
        old->state = READY;
    }

    thread_t *next = old->next;
    int steps = 0;
    while (next->state != READY && steps < MAX_THREADS * 2) {
        next = next->next;
        steps++;
    }

    if (next->state != READY) {
        current = NULL;
        context_switch(&old->esp, caller_esp);
        return;
    }

    next->state = RUNNING;
    current = next;

    context_switch(&old->esp, next->esp);
}

void thread_exit(void) {
    if (current != NULL) {
        current->state = TERMINATED;
    }
    thread_yield();
    for (;;) { }
}

void thread_block(void) {
    if (current != NULL) {
        current->state = BLOCKED;
    }
    thread_yield();
}

void thread_unblock(thread_t *t) {
    if (t != NULL && t->state == BLOCKED) {
        t->state = READY;
    }
}

thread_t *thread_current(void) {
    return current;
}

thread_t *get_thread_table(void) {
    return thread_table;
}

int get_thread_count(void) {
    return thread_count;
}
