#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
// #include "thread.h"

typedef struct queue {
    struct queue *next;
    struct queue *prev;
} queue_t;

int queue_empty(queue_t *head);
void queue_push(queue_t *head, queue_t *element);
queue_t *queue_create();
queue_t *queue_pop(queue_t *head);
// void queue_print(queue_t *head);

#endif
