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
  int alloc_successors;
  int alive;
  int blocked;
  struct thr_descriptor **successors;
} thread_t;

#ifdef REUSE_STACK
typedef struct thr_descriptor_reuse {
  queue_t *descriptors;         // LIFO queue
  int capacity;
  int max_capacity;             // uper bound
} thread_reuse_t;

thread_reuse_t thr_reuse;
#endif

ucontext_t uctx_scheduler;
queue_t *ready_queue;
thread_t main_thread;
long int native_stack_size;
int thread_next_id;

int thread_lib_exit();
int thread_getid();
int thread_yield();
void thread_exit();
void scheduler(void);
int thread_lib_init(int native_threads);
int thread_inc_dependency(int num_deps);
void wrapper_func(void (body)(void *), void *arg);
void free_thread(thread_t *thr);
thread_t *thread_create(void (body)(void *), void *arg, int deps, thread_t *successors[]);
thread_t *thread_self();
thread_t **THREAD_LIST(thread_t *successor);
thread_t **THREAD_LIST2(int nargs, ...);

#endif
