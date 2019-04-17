#ifndef LOCK_H
#define LOCK_H

#include <immintrin.h>

typedef volatile int lock_t;

void lock_init(lock_t *lock);
void lock_acquire(lock_t *lock);
void lock_release(lock_t *lock);

#endif
