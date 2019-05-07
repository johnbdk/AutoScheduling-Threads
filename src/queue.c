#include "queue.h"

queue_t *queue_create() {
    queue_t *queue;

    queue = (queue_t *) malloc(sizeof(queue_t));
    queue->head = (node_t *) malloc(sizeof(node_t));
    queue->head->next = queue->head;
    queue->head->prev = queue->head;
    queue->num_nodes = 0;
    lock_init(&(queue->lock));
    return queue;
}

int queue_empty(queue_t *queue) {
    return (queue->head->next == queue->head) || (queue->head == NULL);
}

void enqueue_head(queue_t *queue, node_t *element) {

    lock_acquire(&(queue->lock));
    if (queue->head == NULL) {
        lock_release(&(queue->lock));
        return;
    }
    
    element->next = queue->head->next;
    element->prev = queue->head;
    queue->head->next->prev = element;
    queue->head->next = element;
    queue->num_nodes++;
    lock_release(&(queue->lock));
}

void enqueue_tail(queue_t *queue, node_t *element) {

    lock_acquire(&(queue->lock));
    if (queue->head == NULL) {
        lock_release(&(queue->lock));
        return;
    }
    
    element->next = queue->head;
    element->prev = queue->head->prev;
    queue->head->prev->next = element;
    queue->head->prev = element;
    queue->num_nodes++;
    lock_release(&(queue->lock));
}

node_t *dequeue_tail(queue_t *queue) {
    node_t *curr;

    lock_acquire(&(queue->lock));
    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        lock_release(&(queue->lock));
        return NULL;
    }

    curr = queue->head->prev;
    curr->prev->next = queue->head;
    queue->head->prev = curr->prev;
    queue->num_nodes--;
    lock_release(&(queue->lock));
    return curr;
}

node_t *dequeue_head(queue_t *queue) {
    node_t *curr;

    lock_acquire(&(queue->lock));
    if ((queue->head->next == queue->head) || (queue->head == NULL)) {
        lock_release(&(queue->lock));
        return NULL;
    }

    curr = queue->head->next;
    queue->head->next = curr->next;
    curr->next->prev = queue->head;
    queue->num_nodes--;
    lock_release(&(queue->lock));
    return curr;
}

node_t *transfer_node(queue_t *queue) {
    node_t *ret_node;

    if (queue->num_nodes < 2) {
        return NULL;
    } 

    lock_acquire(&(queue->lock));
    if (queue->num_nodes < 2) {
        lock_release(&(queue->lock));
        return NULL;
    }

    ret_node = queue->head->next;
    queue->head->next = ret_node->next;
    ret_node->next->prev = queue->head;
    queue->num_nodes--;
    lock_release(&(queue->lock));
    return ret_node;
}

node_t *transfer_nodes(queue_t *dest_queue, queue_t *src_queue, float ratio) {
    int i, nodes_to_transfer;
    node_t *end_node, *start_node, *ret_node;

    if (src_queue->num_nodes < 2) {
        return NULL;
    } 

    lock_acquire(&(src_queue->lock));
    nodes_to_transfer = src_queue->num_nodes*ratio;
    if (nodes_to_transfer == 0) {
        lock_release(&(src_queue->lock));
        return NULL;
    }

    start_node = src_queue->head->next;
    for (i = 0, end_node = src_queue->head; i < nodes_to_transfer; i++, end_node = end_node->next);

    src_queue->head->next = end_node->next;
    end_node->next->prev = src_queue->head;

    src_queue->num_nodes -= nodes_to_transfer;
    lock_release(&(src_queue->lock));

    ret_node = start_node;
    if (start_node == end_node) {
        return ret_node;
    }
    start_node = start_node->next;

    lock_acquire(&(dest_queue->lock));
    dest_queue->head->next = start_node;
    start_node->prev = dest_queue->head;
    end_node->next = dest_queue->head;
    dest_queue->head->prev = end_node;

    dest_queue->num_nodes += nodes_to_transfer - 1;
    lock_release(&(dest_queue->lock));
    return ret_node;
}
