#include "threads.h"

void *wrapper_scheduler(void *id) {
    scheduler(id);
    return NULL;
}

void wrapper_func(void (body)(void *), void *arg) {
    body(arg);
    thread_exit();
}

int thread_lib_init(int native_threads) {

    struct rlimit current_limits;
    long int mask;
    char *native_thread_stack;

    /* Variables for library termination */
    terminate = 0;
    no_native_threads = native_threads;

#ifdef REUSE_STACK
    /* Initialize reuse stack queue */
    thr_reuse.descriptors = queue_create();
    thr_reuse.capacity = 0;
    thr_reuse.max_capacity = 10000;
#endif

    thread_next_id = 0;
    /* Initialize main thread */
    main_thread.id = thread_next_id++;
    main_thread.deps = 0;
    main_thread.alive = 1;
    main_thread.blocked = 0;
    main_thread.self_inced = 0;
    main_thread.next = NULL;
    main_thread.prev = NULL;
    main_thread.stack = NULL;
    main_thread.successors = (thread_t **) malloc(sizeof(thread_t *));
    main_thread.successors[0] = NULL;
    main_thread.kernel_thread_id = 0;

    /* Find the address of the main thread stack */
    getrlimit(RLIMIT_STACK, &current_limits);
    native_stack_size = current_limits.rlim_cur;
    mask = ~(native_stack_size - 1);
    native_thread_stack = (char *) (mask & ((long int)&mask));  // Get address of a variable and find the begining of stack
    main_thread.context.uc_stack.ss_sp = native_thread_stack;   // Save the address inside the main's context, so we can use it later 

    /* Initialize kernel thread 0 (aka main thread) */
    kernel_thr = (kernel_thread_t *) malloc(native_threads*sizeof(kernel_thread_t));
    kernel_thr[0].id = 0;   // Save main kernel thread id, i.e 0 id
    kernel_thr[0].context = (ucontext_t *) malloc(sizeof(ucontext_t));
    kernel_thr[0].ready_queue = queue_create();

    /* Construct the context of kernel thread 0 (i.e target function: scheduler) and save the begining of the address of that stack */
    if (getcontext(kernel_thr[0].context) == -1) {
        handle_error("getcontext");
    }
    kernel_thr[0].context->uc_stack.ss_sp = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);
    kernel_thr[0].context->uc_stack.ss_size = STACK_SIZE;
    makecontext(kernel_thr[0].context, (void *)scheduler, 1, (void *) &(kernel_thr[0].id));

    /* Put main thread into the queue of kernel thread 0 and swap to the scheduler 0 (of kernel thread 0) */ 
    enqueue_head(kernel_thr[0].ready_queue, (node_t *) &main_thread);
    if (swapcontext(&(main_thread.context), kernel_thr[0].context) == -1) {
        handle_error("swapcontext");
    }

    /* Initialize the rest of the kernel threads */
    for (int i = 1; i < native_threads; i++) {
        kernel_thr[i].ready_queue = queue_create();
        kernel_thr[i].id = i;
        kernel_thr[i].context = (ucontext_t *) malloc(sizeof(ucontext_t));
        kernel_thr[i].thr = (pthread_t *) malloc(sizeof(pthread_t));
        pthread_create(kernel_thr[i].thr, NULL, wrapper_scheduler, (void *) &kernel_thr[i].id);
    }
    pthread_setconcurrency(native_threads);
    return 0;
}

thread_t *thread_create(void (body)(void *), void *arg, int deps, thread_t *successors[]) {

    thread_t *thr;

#ifdef REUSE_STACK
    int empty_descriptors = 1;
    thr = (thread_t *) dequeue_tail(thr_reuse.descriptors);
    if (thr != NULL) {
    	empty_descriptors = 0;
        __sync_fetch_and_add(&(thr_reuse.capacity), -1);
    }
    else {
#endif
    thr = (thread_t *) malloc(sizeof(thread_t));
    if (getcontext(&(thr->context)) == -1) {
        handle_error("getcontext");
    }
    thr->stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);    // Allocate alligned memory 8*MEM_SIZE
    thr->successors = NULL;
    memcpy(thr->stack, &thr, sizeof(thread_t *));                   // First 16 bytes are for the pointer pointing to the descriptor
    (thr->context).uc_stack.ss_sp = thr->stack + 16;                // All the other bytes
    (thr->context).uc_stack.ss_size = STACK_SIZE - 16;
    (thr->context).uc_link = NULL;
    thr->alloc_successors = 0;
#ifdef REUSE_STACK
    }
#endif
    
    makecontext(&(thr->context), (void *)wrapper_func, 2, body, arg);
    thr->next = NULL;
    thr->prev = NULL;

    thr->id = __sync_fetch_and_add(&thread_next_id, 1);
    // printf("THREAD CREATE %d\n", thr->id);
    thr->deps = deps;
    thr->self_inced = 0;
    thr->alive = 1;
    thr->blocked = 0;
    thr->num_successors = 0;

    for (int j = 0; successors[j] != NULL; j++) {
        thr->num_successors++;
    }

#ifdef REUSE_STACK
    if (empty_descriptors) {
#endif
        thr->alloc_successors = thr->num_successors;
        thr->successors = (thread_t **) malloc((thr->num_successors)*sizeof(thread_t *));
#ifdef REUSE_STACK
    }
    else {
        if (thr->alloc_successors < thr->num_successors) {
            thread_t **new_successors = (thread_t **) realloc(thr->successors, (thr->num_successors)*sizeof(thread_t *));
            thr->alloc_successors = thr->num_successors;
            thr->successors = new_successors;
        }
    }
#endif
    
    for (int i = 0; i < thr->num_successors; i++) {
        thr->successors[i] = successors[i];
    }

    if (thr->num_successors) {
        thr->kernel_thread_id = successors[0]->kernel_thread_id; 
    }
    else {
        thr->kernel_thread_id = 0; 
    }

    if (!thr->deps) {
        enqueue_tail(kernel_thr[thr->kernel_thread_id].ready_queue, (node_t *) thr);
    }
    return thr;
}

thread_t *thread_self() {
    long int mask;
    thread_t *self;
    char *stack_pointer;

    mask = ~(native_stack_size - 1);
    stack_pointer = (char *) (mask & ((long int)&mask));
    if (stack_pointer == main_thread.context.uc_stack.ss_sp) {
        return &main_thread;
    }

    mask = ~(STACK_SIZE - 1);
    /*get address of a variable and find begining of stack */
    self = *(thread_t **) (mask & ((long int)&mask));

    return self;
}

int thread_getid() {
    thread_t *me;
    
    me = thread_self();
    return me->id;
}

int thread_inc_dependency(int num_deps) {
    thread_t *me;
    
    me = thread_self();
    me->self_inced = 1;
    __sync_fetch_and_add(&(me->deps), num_deps + 1);
    return 0;
}

int thread_yield() {
    thread_t *me;
    
    me = thread_self();
    // printf("THREAD YIELD: yield from thread %d\n", me->id);
    // fflush(stdout);
    
    if (swapcontext(&(me->context), kernel_thr[me->kernel_thread_id].context) == -1) {
        handle_error("swapcontext");
    }
    return 0;
}

void thread_exit() {
    thread_t *me;

    me = thread_self();
    // printf("THREAD EXIT %d\n",me->id);
    // fflush(stdout);
    if (me->alive == 1) {
        for (int i = 0; i < me->num_successors; i++) {
            int curr_deps = __sync_fetch_and_add(&(me->successors[i]->deps), -1);
            if (curr_deps == 1 && !me->successors[i]->blocked) {
                // // printf("THREAD EXIT: thread %d successor %d has 0 deps, adding him in the queue\n", me->id, me->successors[i]->id);
                //flush(stdout);
                enqueue_tail(kernel_thr[me->kernel_thread_id].ready_queue, (node_t *) me->successors[i]);
            }
        }
    }
    me->alive = 0;
    if (me->id) {
        if (swapcontext(&(me->context), kernel_thr[me->kernel_thread_id].context) == -1) {
            handle_error("swapcontext");
        }
    }
}

int thread_lib_exit() {

	printf("THREAD LIB EXIT START\n");
	fflush(stdout);


#ifdef REUSE_STACK
    thread_t *thr;

    thr = (thread_t *) dequeue_head(thr_reuse.descriptors);
    while (thr != NULL) {
        (thr_reuse.capacity)--;
        if (thr->num_successors > 0) {
            free(thr->successors);
        }
        free(thr->stack);
        free(thr);
        thr = (thread_t *) dequeue_head(thr_reuse.descriptors);
    }
#endif

    free(main_thread.successors);
    // free(uctx_scheduler.uc_stack.ss_sp);

    printf("THREAD LIB EXIT END\n");
	fflush(stdout);

    terminate = 1;

    return 0;
}

void free_thread(thread_t *thr) {

#ifdef REUSE_STACK
    if (thr->id) {
        enqueue_tail(thr_reuse.descriptors, (node_t *) thr);
        __sync_fetch_and_add(&(thr_reuse.capacity), 1);
    }
#else
    if (thr->id) {
        if (thr->num_successors > 0) {
            free(thr->successors);
        }
        free(thr->stack);
        free(thr);
    }
#endif
}

void work_stealing(int native_thread) {
    int k, i;
    float stealing = 1/2.;

    for (k = 0, i = ((native_thread + 1) % no_native_threads); k < (no_native_threads - 1); k++, i = ((i + 1) % no_native_threads)) {
        if (transfer_nodes(kernel_thr[native_thread].ready_queue, kernel_thr[i].ready_queue, stealing)) {
            return;
        }
    }
}

void scheduler(void *id) {
    thread_t *running_thread;
    int native_thread = *((int *)id);
    int temp_deps;
    
    while (!terminate) {
        running_thread = (thread_t *) dequeue_tail(kernel_thr[native_thread].ready_queue);
        if (running_thread == NULL) {
            work_stealing(native_thread);
            continue;
        }
        running_thread->kernel_thread_id = native_thread;

        printf("SCHEDULER: run the next thread %d from kernel thread %d, queue: %p\n", running_thread->id, native_thread, kernel_thr[native_thread].ready_queue);
        fflush(stdout);

        if (swapcontext(kernel_thr[native_thread].context, &(running_thread->context)) == -1) {
            handle_error("swapcontext");
        }

        printf("SCHEDULER: return of thread %d on kernel thread %d, queue: %p\n", running_thread->id, native_thread, kernel_thr[native_thread].ready_queue);
        fflush(stdout);
        if (running_thread->alive == 0) {
            free_thread(running_thread);
        }
        else {
            if (running_thread->self_inced) {
                temp_deps = __sync_fetch_and_add(&running_thread->deps, -1);
                running_thread->self_inced = 0;
            }
            else {
                temp_deps = 1;
            }
            if (temp_deps == 1 && !running_thread->blocked) {
                enqueue_head(kernel_thr[native_thread].ready_queue, (node_t *) running_thread);
            }
        }
        // Push myself to queue from yield so we are protected if another native thread take us before we swapcontex
    }
    if (!native_thread) {
        for (int i = 1; i < no_native_threads; i++) {
            pthread_join(*(kernel_thr[i].thr), NULL);
        }
    }
}

thread_t **THREAD_LIST(thread_t *successor) {
    thread_t **array_of_successors;
    
    array_of_successors = (thread_t **) malloc(2*sizeof(thread_t *));
    array_of_successors[0] = successor;
    array_of_successors[1] = NULL;
    return array_of_successors;
}

thread_t **THREAD_LIST2(int nargs, ...) {
    thread_t **array_of_successors;
    va_list ap;
    
    array_of_successors = (thread_t **) malloc((nargs+1)*sizeof(thread_t *));
    array_of_successors[nargs] = NULL;

    va_start(ap, nargs);
    for (int i = 0; i < nargs; i++) {
        array_of_successors[i] = va_arg(ap, thread_t *);
    }
    va_end(ap);
    return array_of_successors;
}
