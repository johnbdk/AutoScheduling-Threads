#include <stdio.h>
#include <unistd.h>

#include "threads.h"

void work(void *arg) {
    int i;
    int id;

    id = thread_getid();

    for (i=0; i<5; i++) {
        printf("%s id=%d: %d, %p\n", (char *)arg, id, i, &i);
        fflush(stdout);
        // sleep(1);
        thread_yield();
    }

    thread_exit();
}

int main(int argc, char *argv[]) {
    thread_t t1;
    thread_t t2;
    thread_t t3;
    thread_t t4;


    if(argc < 2){
        printf("argv[1] is empty\n");
        return 1;
    }
    reuse_stacks = (int) strtol(argv[1], NULL, 10);
    /*
      t1 --> t2
      t1 --> t3
      t1 --> t4
     */

    thread_lib_init(1);

    thread_create(&t4, work, "t4", 1, THREAD_LIST2(1,thread_self()));
    thread_create(&t3, work, "t3", 1, THREAD_LIST2(1,thread_self()));
    thread_create(&t2, work, "t2", 1, THREAD_LIST2(1,thread_self()));
    thread_create(&t1, work, "t1", 0, THREAD_LIST2(3,&t2,&t3,&t4));

    thread_inc_dependency(3);

    thread_yield();
    printf("in main, id = %d\n", thread_getid());

    thread_exit();

    printf("OK\n");

    thread_lib_exit();

    return 0;
}
