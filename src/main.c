#include <stdio.h>
#include <unistd.h>

#include "threads.h"

// void work2(void *arg) {
//     int id;

//     id = thread_getid();
//     printf("%s id=%d\n", (char *)arg, id);
//     fflush(stdout);

//     thread_exit();
// }

void work(void *arg) {
    int i;
    int id;

    id = thread_getid();

    // for (i=0; i<5; i++) {
    //     printf("%s id=%d: %d, %p\n", (char *)arg, id, i, &i);
    //     fflush(stdout);
    //     // sleep(1);
    //     thread_yield();
    // }
    printf("%s id=%d\n", (char *)arg, id);
    fflush(stdout);

    // thread_inc_dependency(1);
    // thread_create(work2, "GGG", 0, THREAD_LIST(thread_self()));
    // thread_yield();
    thread_exit();
}

int main(int argc, char *argv[]) {
    thread_t *t1;
    thread_t *t2;
    thread_t *t3;
    thread_t *t4;

    /*
      t1 --> t2
      t1 --> t3
      t1 --> t4
     */

    thread_lib_init(4);
    thread_inc_dependency(4);

    t4 = thread_create(work, "t4", 0, THREAD_LIST(thread_self()));
    t3 = thread_create(work, "t3", 0, THREAD_LIST(thread_self()));
    t2 = thread_create(work, "t2", 0, THREAD_LIST(thread_self()));
    t1 = thread_create(work, "t1", 0, THREAD_LIST(thread_self()));


    thread_yield();
    // sleep(10);
    printf("in main, id = %d\n", thread_getid());
    fflush(stdout);
    sleep(5);
    thread_exit();

    printf("OK\n");

    thread_lib_exit();

    return 0;
}