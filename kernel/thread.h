#ifndef THREAD_H
#define THREAD_H

#include "../include/types.h"
#include "../include/process.h"

#define MAX_THREADS   16
#define STACK_SIZE    4096

typedef struct thread {
    uint32_t        tid;
    proc_state_t    state;
    uint32_t        esp;
    uint32_t        eip;
    uint32_t        stack[STACK_SIZE / 4];
    char            name[16];
    struct thread  *next;
    struct thread  *next_wait;
} thread_t;

void       thread_init(void);
thread_t  *thread_create(void (*entry)(void *arg), void *arg);
void       thread_yield(void);
void       thread_exit(void);
void       thread_block(void);
void       thread_unblock(thread_t *t);
thread_t  *thread_current(void);
thread_t  *get_thread_table(void);
int        get_thread_count(void);

#endif
