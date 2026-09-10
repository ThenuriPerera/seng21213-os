#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"

#define MAX_PROCESSES 16
#define STACK_SIZE    4096

typedef enum { READY, RUNNING, BLOCKED, TERMINATED } proc_state_t;

typedef struct pcb {
    uint32_t      pid;
    proc_state_t  state;
    uint32_t      esp;          /* Saved stack pointer */
    uint32_t      eip;          /* Saved instruction pointer / entry point */
    uint32_t      stack[STACK_SIZE / 4];
    char          name[16];
    struct pcb   *next;         /* For linked-list ready queue */
} pcb_t;

void   process_init(void);
pcb_t *process_create(void (*entry)(void));
void   process_yield(void);
void   process_exit(void);
void   scheduler_tick(void);

pcb_t *get_process_table(void);
int    get_process_count(void);

extern void context_switch(uint32_t *old_sp, uint32_t new_sp);

#endif /* PROCESS_H */
