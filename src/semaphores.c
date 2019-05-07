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
        enqueue_head(blocking_queue, (node_t *)curr_thr);
        thread_yield();
    }
    s->val--;
    return 0;
}

int sem_up(sem_t *s) {
    thread_t *curr_thr;
    
    curr_thr = (thread_t *) dequeue_tail(blocking_queue);
    if (curr_thr != NULL) {
        if (curr_thr->deps == 0) {
            curr_thr->blocked = 0;
            enqueue_head(kernel_thr[curr_thr->kernel_thread_id].vars.ready_queue, (node_t *)curr_thr);
        }
    }
    return 0;
}

int sem_destroy(sem_t *s) {
    
    free(blocking_queue);
    return 0;
}
