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

    /* Build the stack so context_switch's `ret` lands on `entry` with `arg`
     * sitting exactly where cdecl expects a first parameter, and a fake
     * return address (thread_exit) so a normal `return;` inside entry()
     * cleans the thread up automatically instead of jumping into garbage. */
    *(--stack_top) = (uint32_t)arg;
    *(--stack_top) = (uint32_t)thread_exit;   /* fake return address */
    *(--stack_top) = (uint32_t)entry;         /* what `ret` in context_switch jumps to */

    /* Dummy PUSHAD-order registers so POPAD in switch.asm resolves them */
    *(--stack_top) = 0; /* EAX */
    *(--stack_top) = 0; /* ECX */
    *(--stack_top) = 0; /* EDX */
    *(--stack_top) = 0; /* EBX */
    *(--stack_top) = 0; /* dummy ESP */
    *(--stack_top) = 0; /* EBP */
    *(--stack_top) = 0; /* ESI */
    *(--stack_top) = 0; /* EDI */

    t->tid   = next_tid++;
    t->state = READY;
    t->esp   = (uint32_t)stack_top;
    t->eip   = (uint32_t)entry;
    t->name[0] = '\0';
    t->next  = NULL;

    /* Insert into the circular ready list, same pattern as process.c */
    if (ready_head == NULL) {
        ready_head = t;
        t->next = t;
    } else {
        thread_t *tail = ready_head;
        while (tail->next != ready_head) {
            tail = tail->next;
        }
        tail->next = t;
        t->next = ready_head;
    }

    thread_count++;
    return t;
}

void thread_yield(void) {
    if (ready_head == NULL) {
        return;
    }

    if (current == NULL) {
        current = ready_head;
        current->state = RUNNING;
        uint32_t dummy_old_esp;
        context_switch(&dummy_old_esp, current->esp);
        return;
    }

    thread_t *old  = current;
    thread_t *next = old->next;

    while (next->state != READY && next != old) {
        next = next->next;
    }

    if (old->state == RUNNING) {
        old->state = READY;
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
    /* Never returns */
    for (;;) { }
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
