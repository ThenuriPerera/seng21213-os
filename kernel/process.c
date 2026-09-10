/* =============================================================================
 * SENG21213-OS :: Process table & round-robin scheduler
 * File   : kernel/process.c
 * ============================================================================*/
#include "../include/types.h"
#include "../include/process.h"

static pcb_t process_table[MAX_PROCESSES];
static int   process_count = 0;
static uint32_t next_pid = 1;

static pcb_t *ready_head = NULL;   /* head of the circular ready list */
static pcb_t *current    = NULL;   /* currently running process */

void process_init(void) {
    process_count = 0;
    next_pid = 1;
    ready_head = NULL;
    current = NULL;
}

pcb_t *process_create(void (*entry)(void)) {
    if (process_count >= MAX_PROCESSES) {
        return NULL;
    }

    pcb_t *p = &process_table[process_count];

    /* Stack grows downward; stack[] array's last element is the "top" */
    uint32_t *stack_top = &p->stack[STACK_SIZE / 4];

    /* Push return address (entry point) first */
    *(--stack_top) = (uint32_t)entry;

    /* Push dummy PUSHAD-order registers so POPAD in switch.asm resolves them */
    *(--stack_top) = 0; /* EAX */
    *(--stack_top) = 0; /* ECX */
    *(--stack_top) = 0; /* EDX */
    *(--stack_top) = 0; /* EBX */
    *(--stack_top) = 0; /* dummy ESP (popped and discarded) */
    *(--stack_top) = 0; /* EBP */
    *(--stack_top) = 0; /* ESI */
    *(--stack_top) = 0; /* EDI */

    p->pid   = next_pid++;
    p->state = READY;
    p->esp   = (uint32_t)stack_top;
    p->eip   = (uint32_t)entry;
    p->name[0] = '\0';
    p->next  = NULL;

    /* Insert into the circular ready list */
    if (ready_head == NULL) {
        ready_head = p;
        p->next = p; /* points to itself, circular list of one */
    } else {
        pcb_t *tail = ready_head;
        while (tail->next != ready_head) {
            tail = tail->next;
        }
        tail->next = p;
        p->next = ready_head;
    }

    process_count++;
    return p;
}

void process_yield(void) {
    scheduler_tick();
}

void process_exit(void) {
    if (current != NULL) {
        current->state = TERMINATED;
    }
    scheduler_tick();
}

void scheduler_tick(void) {
    if (ready_head == NULL) {
        return; /* nothing to schedule yet */
    }

    if (current == NULL) {
        /* First ever switch: just start at the head, no old context to save */
        current = ready_head;
        current->state = RUNNING;
        uint32_t dummy_old_esp;
        context_switch(&dummy_old_esp, current->esp);
        return;
    }

    pcb_t *old = current;
    pcb_t *next = old->next;

    /* Skip anything not READY/RUNNING (e.g. TERMINATED, BLOCKED) */
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

pcb_t *get_process_table(void) {
    return process_table;
}

int get_process_count(void) {
    return process_count;
}
