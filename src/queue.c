#include "queue.h"
#include "threads.h"
queue_t *queue_create() {
    queue_t *head;

    head = (queue_t *) malloc(sizeof(queue_t));
    head->next = head;
    head->prev = head;
    return head;
}

int queue_empty(queue_t *head) {
    return (head->next == head) || (head == NULL);
}

void enqueue_head(queue_t *head, queue_t *element) {    // put in head (high priority)

    if (head == NULL) {
        return;
    }
    
    element->next = head->next;
    element->prev = head;
    head->next->prev = element;
    head->next = element;
}

void enqueue_tail(queue_t *head, queue_t *element) {    // put in tail (low priority)

    if (head == NULL) {
        return;
    }
    
    element->next = head;
    element->prev = head->prev;
    head->prev->next = element;
    head->prev = element;
}

queue_t *dequeue_tail(queue_t *head) {                  // take from tail (high priority)
    queue_t *curr;
    
    if (queue_empty(head)) {
        return NULL;
    }


    curr = head->prev;
    curr->prev->next = head;
    head->prev = curr->prev;
    return curr;
}

queue_t *dequeue_head(queue_t *head) {                  // take from head (low priority)
    queue_t *curr;
    
    if (queue_empty(head)) {
        return NULL;
    }

    curr = head->next;
    head->next = curr->next;
    curr->next->prev = head;
    return curr;
}

void print_queue(queue_t *head){
    queue_t *curr;
    thread_t *thr;

    printf("~~~~~\n");
    for (curr = head->next; curr != head; curr = curr->next) {
        thr = (thread_t *)curr;
        printf("%d ", thr->id);
    }
    printf("\n~~~~~\n");
}