#include <stdio.h>
#include <unistd.h>

#include "threads.h"

void work(void *arg) {
    int id;

    id = thread_getid();

    printf("%s id=%d\n", (char *)arg, id);
    fflush(stdout);


    thread_exit();
}

int main(int argc, char *argv[]) {

    thread_lib_init(4);
    thread_inc_dependency(4);

    thread_create(work, "t4", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t3", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t2", 0, THREAD_LIST(thread_self()));
    thread_create(work, "t1", 0, THREAD_LIST(thread_self()));

    thread_yield();
    printf("in main, id = %d\n", thread_getid());
    fflush(stdout);
    thread_exit();

    printf("OK\n");

    thread_lib_exit();

    return 0;
}
