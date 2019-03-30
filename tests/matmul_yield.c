#include <stdio.h>
#include "threads.h"

#define SIZE	1024
double A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];

void thread_func(void *arg) {
    unsigned long row = (unsigned long)arg;
    unsigned long col, k;

    for(col = 0; col < SIZE; col++) {
        C[row][col] = 0;
        for (k = 0; k < SIZE; k++)
            C[row][col] += A[row][k]*B[k][col];
        thread_yield();
    }
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

    thread_t thread[SIZE];

    if(argc < 2){
        printf("argv[1] is empty\n");
        return 1;
    }
    reuse_stacks = (int) strtol(argv[1], NULL, 10);

    init_arrays();
    thread_lib_init(1);
    myself = thread_self();
    for (i = 0; i < SIZE; i++)
        thread_create(&thread[i], thread_func, (void *)i, 0, THREAD_LIST(myself));

    thread_inc_dependency(SIZE);
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
