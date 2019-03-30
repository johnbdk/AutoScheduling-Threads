#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <stdlib.h>
#include "threads.h"
#include "queue.h"

typedef struct semaphore {
    int val;
    int max_val;
} sem_t;

int sem_init(sem_t *s, int val);
int sem_down(sem_t *s);
int sem_up(sem_t *s);
int sem_destroy(sem_t *s);

queue_t *blocking_queue;

#endif
