#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include "lock.h"

typedef struct node {
    struct node *next;
    struct node *prev;
} node_t;

typedef struct queue {
	node_t *head;
    lock_t lock;
    volatile int num_nodes;
} queue_t;

int queue_empty(queue_t *head);
int transfer_nodes(queue_t *dest_queue, queue_t *src_queue, float ratio);
void print_queue(queue_t *head);
void enqueue_head(queue_t *head, node_t *element);
void enqueue_tail(queue_t *head, node_t *element);
queue_t *queue_create();
node_t *dequeue_head(queue_t *head);
node_t *dequeue_tail(queue_t *head);

#endif
