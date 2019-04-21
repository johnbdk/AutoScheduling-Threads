#include "queue.h"

queue_t *queue_create() {
    queue_t *queue;

    queue = (queue_t *) malloc(sizeof(queue_t));
    queue->head = (node_t *) malloc(sizeof(node_t));
    queue->head->next = queue->head;
    queue->head->prev = queue->head;
    lock_init(&(queue->lock));

    return queue;
}

int queue_empty(queue_t *queue) {
    return (queue->head->next == queue->head) || (queue->head == NULL);
}

void enqueue_head(queue_t *queue, node_t *element) {

    lock_acquire(&(queue->lock));
    if (queue->head == NULL) {
        // printf("hereee1\n");
        lock_release(&(queue->lock));
        return;
    }
    
    element->next = queue->head->next;
    element->prev = queue->head;
    queue->head->next->prev = element;
    queue->head->next = element;
    lock_release(&(queue->lock));
}

void enqueue_tail(queue_t *queue, node_t *element) {

    lock_acquire(&(queue->lock));
    if (queue->head == NULL) {
        // printf("hereee2\n");
        lock_release(&(queue->lock));
        return;
    }
    
    element->next = queue->head;
    element->prev = queue->head->prev;
    queue->head->prev->next = element;
    queue->head->prev = element;
    lock_release(&(queue->lock));
}

node_t *dequeue_tail(queue_t *queue) {
    node_t *curr;

    lock_acquire(&(queue->lock));
    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        lock_release(&(queue->lock));
        // printf("hereee3\n");
        return NULL;
    }

    curr = queue->head->prev;
    curr->prev->next = queue->head;
    queue->head->prev = curr->prev;
    lock_release(&(queue->lock));

    return curr;
}

node_t *dequeue_head(queue_t *queue) {
    node_t *curr;

    lock_acquire(&(queue->lock));
    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        lock_release(&(queue->lock));
        // printf("hereee4\n");
        return NULL;
    }

    curr = queue->head->next;
    queue->head->next = curr->next;
    curr->next->prev = queue->head;
    lock_release(&(queue->lock));

    return curr;
}

node_t *dequeue_head_no_lock(queue_t *queue) {
    node_t *curr;

    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        return NULL;
    }

    curr = queue->head->next;
    queue->head->next = curr->next;
    curr->next->prev = queue->head;

    return curr;
}
