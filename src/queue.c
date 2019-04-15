#include "queue.h"
#include "threads.h"

queue_t *queue_create() {
    queue_t *head;

    head = (queue_t *) malloc(sizeof(queue_t));
    head->next = head;
    head->prev = head;
    lock_init(&(head->lock));

    return head;
}

int queue_empty(queue_t *head) {
    return (head->next == head) || (head == NULL);
}

void enqueue_head(queue_t *head, queue_t *element) {    // put in head (high priority)

	lock_acquire(&(head->lock));
    if (head == NULL) {
    	lock_release(&(head->lock));
        return;
    }
    // printf("~~ EN-QUEUE: %p\n", element);
    // fflush(stdout);
    
    element->next = head->next;
    element->prev = head;
    head->next->prev = element;
    head->next = element;
    lock_release(&(head->lock));
}

void enqueue_tail(queue_t *head, queue_t *element) {    // put in tail (low priority)

	lock_acquire(&(head->lock));
    if (head == NULL) {
        // printf("hereee\n");
		lock_release(&(head->lock));
        return;
    }
 //    printf("~~ EN-QUEUE: %p\n", element);
	// fflush(stdout);

    element->next = head;
    element->prev = head->prev;
    head->prev->next = element;
    head->prev = element;
    lock_release(&(head->lock));
}

queue_t *dequeue_tail(queue_t *head) {                  // take from tail (high priority)
    queue_t *curr;

    lock_acquire(&(head->lock));
    if ( (head->next == head) || (head == NULL) ) {
		lock_release(&(head->lock));
        return NULL;
    }
    // print_queue(head);

    curr = head->prev;
    curr->prev->next = head;
    head->prev = curr->prev;

    // printf("-- DE-QUEUE: %p\n", curr);
    // fflush(stdout);
	lock_release(&(head->lock));

    return curr;
}

queue_t *dequeue_head(queue_t *head) {                  // take from head (low priority)
    queue_t *curr;

    lock_acquire(&(head->lock));
    if ( (head->next == head) || (head == NULL) ) {
    	lock_release(&(head->lock));	
        return NULL;
    }
    // print_queue(head);

    curr = head->next;
    head->next = curr->next;
    curr->next->prev = head;

    // printf("-- DE-QUEUE: %p\n", curr);
    // fflush(stdout);
    lock_release(&(head->lock));

    return curr;
}

void print_queue(queue_t *head){
    queue_t *curr;
    thread_t *thr;

    // lock_acquire(&(head->lock));
    printf("~~~~~\n");
    for (curr = head->next; curr != head; curr = curr->next) {
        thr = (thread_t *)curr;
        printf("%d ", thr->id);
    }
    printf("\n~~~~~\n");
    // lock_release(&(head->lock));
}