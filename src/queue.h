#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include "lock.h"

typedef struct queue {
    struct queue *next;
    struct queue *prev;
    lock_t lock;
} queue_t;

int queue_empty(queue_t *head);
void print_queue(queue_t *head);
void enqueue_head(queue_t *head, queue_t *element);
void enqueue_tail(queue_t *head, queue_t *element);
queue_t *queue_create();
queue_t *dequeue_head(queue_t *head);
queue_t *dequeue_tail(queue_t *head);

void enqueue_head_no_lock(queue_t *head, queue_t *element);
queue_t *dequeue_head_no_lock(queue_t *head);

#endif