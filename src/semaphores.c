#include "semaphores.h"

int sem_init(sem_t *s, int val) {
    
    s->max_val = val;
    s->val = val;
    
    blocking_queue = queue_create();
    return 0;
}

int sem_down(sem_t * s) {
    thread_t *curr_thr;

    if (s->val == 0) {
        curr_thr = thread_self();
        curr_thr->blocked = 1;
        queue_push(blocking_queue, (queue_t *)curr_thr);
        thread_yield();
    }
    s->val--;
    return 0;
}

int sem_up(sem_t *s) {
    thread_t *curr_thr;
    
    curr_thr = (thread_t *) queue_pop(blocking_queue);
    if (curr_thr != NULL) {
        if (curr_thr->deps == 0) {
            curr_thr->blocked = 0;
            queue_push(ready_queue, (queue_t *)curr_thr);
        }
    }
    return 0;
}

int sem_destroy(sem_t *s) {
    
    free(blocking_queue);
    return 0;
}
