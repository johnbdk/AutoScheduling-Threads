#include "lock.h"

void lock_init(lock_t *lock) {
    *lock = 0;
}

void lock_acquire(lock_t *lock) {
    int res;

    do {
        while (*lock) {
            _mm_pause();
        } 
        res = __sync_bool_compare_and_swap(lock, 0, 1);
    } while (!res);
    
}

void lock_release(lock_t *lock) {
    *lock = 0;
}