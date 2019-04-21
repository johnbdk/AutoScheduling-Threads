#ifndef THREAD_H
#define THREAD_H

#define _GNU_SOURCE

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <sched.h>
#include <signal.h>
#include "queue.h"
#include <pthread.h>
#include "lock.h"

#define PAGE sysconf(_SC_PAGE_SIZE)
#define STACK_SIZE sysconf(_SC_PAGE_SIZE)*8
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct thr_descriptor {
  struct thr_descriptor *next, *prev;
  int id;
  char *stack;
  ucontext_t context;
  volatile int deps;
  int self_inced;       //
  int num_successors;
  int alloc_successors;
  int alive;
  int blocked;
  int kernel_thread_id;
  struct thr_descriptor **successors;
} thread_t;

typedef struct kernel_thread {
  int pid;
  queue_t *ready_queue;
  pthread_t *thr;
  ucontext_t *context;  // padding
  volatile int num_threads;
} kernel_thread_t;

#ifdef REUSE_STACK
typedef struct thr_descriptor_reuse {
  queue_t *descriptors;         // LIFO queue
  int capacity;
  int max_capacity;             // uper bound
} thread_reuse_t;

thread_reuse_t thr_reuse;
#endif

long int native_stack_size;
volatile int terminate;
int no_native_threads;
volatile int thread_next_id;
thread_t main_thread;
ucontext_t uctx_scheduler;
kernel_thread_t *kernel_thr;

int thread_getid();
int thread_yield();
int thread_lib_exit();
int thread_lib_init(int native_threads);
int thread_inc_dependency(int num_deps);
void work_stealing(int native_thread);
void thread_exit();
void scheduler(void *id);
void free_thread(thread_t *thr);
void *wrapper_scheduler(void *id);
void create_kernel_thread(kernel_thread_t *thr);
void wrapper_func(void (body)(void *), void *arg);
thread_t *thread_self();
thread_t **THREAD_LIST2(int nargs, ...);
thread_t **THREAD_LIST(thread_t *successor);
thread_t *thread_create(void (body)(void *), void *arg, int deps, thread_t *successors[]);

#endif