#include "queue.h"

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

void queue_push(queue_t *head, queue_t *element) {

    if (head == NULL) {
        // printf("Queue has not been created\n");
        return;
    }
    
    element->next = head->next;
    element->prev = head;
    head->next->prev = element;
    head->next = element;
}

queue_t *queue_pop(queue_t *head) {
    queue_t *curr;
    
    if (queue_empty(head)) {
        // printf("Queue has not any elements\n");
        return NULL;
    }

    curr = head->prev;
    curr->prev->next = head;
    head->prev = curr->prev;
    return curr;
}

// void queue_print(queue_t *head) {
//     thr_t *tmp;
//     tmp = (thr_t *) head->next;
//     printf("Queue has the following elements\n");
//     while( (queue_t *)tmp != head ){
//         printf("%d\t",tmp->id);
// //         printf("%d\n",tmp->id);
//         tmp = tmp->next;
//     }
//     printf("\n");
// }
