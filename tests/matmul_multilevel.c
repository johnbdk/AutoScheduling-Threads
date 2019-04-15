#include <stdio.h>
#include "threads.h"

#define SIZE	1024
#define CHUNK	128
double A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];

union args {
    void *arg;
    int indices[2];
};

void worker_func(void *arg) {
    unsigned long row = ((union args)arg).indices[0];
    unsigned long col_start = ((union args)arg).indices[1];
    unsigned long col, k;

//    printf("Thread %d working row %lu, col_start %lu\n", thread_getid(), row, col_start);
    for(col = col_start; col < col_start+CHUNK; col++) {
        C[row][col] = 0;
        for (k = 0; k < SIZE; k++) {
            C[row][col] += A[row][k]*B[k][col];
        }
//        thread_yield();
    }
//    printf("Thread %d done\n", thread_getid());
}

void thread_func(void *arg) {
    unsigned long row = (unsigned long)arg;
    unsigned long col;
    union args argument;

    thread_t *self = thread_self();

//    printf("Thread %d creating threads\n", thread_getid());
    thread_inc_dependency(SIZE/CHUNK);
    argument.indices[0] = row;
    for(col = 0; col < SIZE; col = col+CHUNK) {
        argument.indices[1] = col;
        thread_create(worker_func, argument.arg, 0, THREAD_LIST(self));
    }
//    printf("Thread %d done creating threads\n", thread_getid());
    thread_yield();
}


void init_arrays() {
    unsigned long i, j;
    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++) {
            A[i][j] = B[i][j] = (i == j);
        }
}

int main (int argc, char *argv[]) {
    thread_t *myself;
    unsigned long i, j;

    init_arrays();
    thread_lib_init(1);
    myself = thread_self();
    thread_inc_dependency(SIZE);
    for (i = 0; i < SIZE; i++)
        thread_create(thread_func, (void *)i, 0, THREAD_LIST(myself));

    thread_yield();

    printf("Finished\n");
    thread_lib_exit();
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            if (i != j) {
                if (C[i][j] != 0.0)
                    printf("Error (%lu, %lu)\n", i, j);
            }
            else if (C[i][j] != 1.0)
                printf("ErrorB (%lu, %lu)\n", i, j);
        }
    }
    return(0);
}
