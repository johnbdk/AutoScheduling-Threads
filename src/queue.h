#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include "lock.h"

typedef struct node {
    struct node *next;
    struct node *prev;
} node_t;

typedef struct queue {
	node_t *head;
    lock_t lock;
} queue_t;

int queue_empty(queue_t *queue);
void print_queue(queue_t *queue);
void enqueue_head(queue_t *queue, node_t *element);
void enqueue_tail(queue_t *queue, node_t *element);
queue_t *queue_create();
node_t *dequeue_head(queue_t *queue);
node_t *dequeue_tail(queue_t *queue);

#endif
