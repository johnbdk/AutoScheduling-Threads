#include <stdio.h>
#include <unistd.h>
#include "sem.h"
#include "threads.h"

sem_t my_semaphore;

void work(void *arg) {
    int i;
    int id;

    id = thread_getid();

    sem_down(&my_semaphore);
    for (i=0; i<5; i++) {
        printf("%s id=%d: %d, %p\n", (char *)arg, id, i, &i);
        fflush(stdout);
        // sleep(1);
        thread_yield();
    }
    sem_up(&my_semaphore);

    thread_exit();
}

int main(int argc, char *argv[]) {
    thread_t t1;
    thread_t t2;
    thread_t t3;
    thread_t t4;

    thread_lib_init(1);
    
    sem_init(&my_semaphore,1);

    thread_create(&t4, work, "t4", 0, THREAD_LIST2(1,thread_self()));
    thread_create(&t3, work, "t3", 0, THREAD_LIST2(1,thread_self()));
    thread_create(&t2, work, "t2", 0, THREAD_LIST2(1,thread_self()));
    thread_create(&t1, work, "t1", 0, THREAD_LIST2(1,thread_self()));

    thread_inc_dependency(4);

    thread_yield();
    printf("in main, id = %d\n", thread_getid());

    thread_exit();

    printf("OK\n");

    sem_destroy(&my_semaphore);
    thread_lib_exit();

    return 0;
}
