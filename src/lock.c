#include "lock.h"

void lock_init(lock_t *lock) {
	*lock = 0;
}

void lock_acquire(lock_t *lock) {
	while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
    	sched_yield();
    	// while(*p){
    	// 	_mm_pause();
    	// }
    }
}

void lock_release(lock_t *lock) {
	// asm volatile (
	// 	"movl $0, %0;" 
 //    	:"=r"(*lock)
 //    );
    *lock = 0;
}
