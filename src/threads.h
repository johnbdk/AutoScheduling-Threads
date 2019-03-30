#ifndef THREAD_H
#define THREAD_H

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/resource.h>
#include "queue.h"

#define PAGE sysconf(_SC_PAGE_SIZE)
#define STACK_SIZE sysconf(_SC_PAGE_SIZE)*8
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct thr_descriptor {
  struct thr_descriptor *next, *prev;
  int id;
  char *stack;
  ucontext_t context;
  int deps;
  int num_successors;
  int alive;
  int blocked;
  struct thr_descriptor **successors;
} thread_t;

ucontext_t uctx_scheduler;
queue_t *ready_queue;
thread_t main_thread;
long int native_stack_size;
int thread_next_id;

int thread_lib_init(int native_threads);
int thread_lib_exit();
int thread_getid();
int thread_create(thread_t *thr, void (body)(void *), void *arg, int deps, thread_t *successors[]);
int thread_yield();
int thread_inc_dependency(int num_deps);
int thread_lib_exit();
void thread_exit();
void scheduler(void);
void wrapper_func(void (body)(void *), void *arg);
void free_thread(thread_t *thr);
thread_t *thread_self();
thread_t **THREAD_LIST(thread_t *successor);
thread_t **THREAD_LIST2(int nargs, ...);

#endif
