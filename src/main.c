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
    int id;

    id = thread_getid();

    // for (i=0; i<5; i++) {
    //     printf("%s id=%d: %d, %p\n", (char *)arg, id, i, &i);
    //     fflush(stdout);
    //     // sleep(1);
    //     thread_yield();
    // }
    // printf("%s id=%d\n", (char *)arg, id);
    // fflush(stdout);

    // thread_inc_dependency(1);
    // thread_create(work2, "GGG", 0, THREAD_LIST(thread_self()));
    // thread_yield();
    thread_exit();
}

int main(int argc, char *argv[]) {
    /*
      t1 --> t2
      t1 --> t3
      t1 --> t4
      t1 --> t5
      t1 --> t6
     */

    thread_lib_init(4);
    thread_inc_dependency(12);

    thread_create(work, "t12", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t11", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t10", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t9", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t8", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t7", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t6", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t5", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t4", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t3", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t2", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t1", 0, THREAD_LIST(thread_self()));

    thread_yield();
    // sleep(10);
    printf("in main, id = %d\n", thread_getid());
    fflush(stdout);
    // sleep(5);
    thread_exit();

    thread_lib_exit();
    
    printf("OK\n");

    return 0;
}