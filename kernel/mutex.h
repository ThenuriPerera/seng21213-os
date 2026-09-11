#ifndef MUTEX_H
#define MUTEX_H

#include "../include/types.h"
#include "thread.h"

typedef struct {
    volatile int locked;
    thread_t     *wait_head;   /* FIFO queue of blocked threads */
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);

#endif
