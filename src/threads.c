#include "threads.h"

void create_kernel_thread(kernel_thread_t *kernel_thr) {

    kernel_thr->stack = (char *) malloc(STACK_SIZE*sizeof(char));
    printf("@@@ %d\n",kernel_thr->id);
	fflush(stdout);
    kernel_thr->pid = clone(&wrapper_scheduler, kernel_thr->stack + STACK_SIZE, CLONE_VM | SIGCHLD, (void *)&kernel_thr->id);       //CLONE_VM
}

int wrapper_scheduler(void *id) {
	// printf("### %d\n",*((int *) id));
	// fflush(stdout);
    scheduler(id);
    return 1;
}

int thread_lib_init(int native_threads) {

    struct rlimit current_limits;
    long int mask;
    char *native_thread_stack;
    static ucontext_t uctx_main;

    thread_next_id = 0;
    no_threads = 0;
    terminate = 0;
    no_native_threads = native_threads;

#ifdef REUSE_STACK
    thr_reuse.descriptors = queue_create();
    thr_reuse.capacity = 0;
    thr_reuse.max_capacity = 10000;
#endif

    main_thread.id = thread_next_id;
    thread_next_id++;
    main_thread.deps = 0;
    main_thread.alive = 1;
    main_thread.blocked = 0;
    main_thread.self_inced = 0;
    main_thread.next = NULL;
    main_thread.prev = NULL;
    main_thread.stack = NULL;
    main_thread.successors = NULL;
    main_thread.context = uctx_main;
    main_thread.kernel_thread_id = 0;
    // printf("uctx_main is %p\n", &uctx_scheduler);

    getrlimit(RLIMIT_STACK, &current_limits);
    native_stack_size = current_limits.rlim_cur;
    
    mask = ~(native_stack_size - 1);
    native_thread_stack = (char *) (mask & ((long int)&mask));       // get address of a variable and find begining of stack
    main_thread.context.uc_stack.ss_sp = native_thread_stack;

    kernel_thr = (kernel_thread_t *) malloc(native_threads*sizeof(kernel_thread_t));    // REMEMBER TO FREE
    kernel_thr[0].id = 0;                                                               // SAVE MAIN ID, i.e 0 id
    kernel_thr[0].pid = getpid();
    kernel_thr[0].num_threads = 1;
    kernel_thr[0].stack = native_thread_stack;                                          // FOR COMPLECITY
    kernel_thr[0].context = &uctx_scheduler;                                            // FOR COMPLECITY 
    kernel_thr[0].ready_queue = queue_create();

    if (getcontext(&uctx_scheduler) == -1) {
        handle_error("getcontext");
    }
    char *scheduler_stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);     // ALLOCATE ALIGNED MEMORY 8*MEM_SIZE
    uctx_scheduler.uc_stack.ss_sp = scheduler_stack;                            // ALL THE OTHER BYTES
    uctx_scheduler.uc_stack.ss_size = STACK_SIZE;
    makecontext(&(uctx_scheduler), (void *)scheduler, 1, (void *) &kernel_thr[0].id);

    for (int i = 1; i < native_threads; i++) {
        kernel_thr[i].ready_queue = queue_create();
        kernel_thr[i].id = i;
        kernel_thr[i].num_threads = 0;
        kernel_thr[i].context = (ucontext_t *) malloc(sizeof(ucontext_t));  // REMEMBER TO FREE
        printf("ON CREATE: %d - %p\n",i, kernel_thr[i].context);
        fflush(stdout);
        create_kernel_thread(&kernel_thr[i]);
    }

    enqueue_head(kernel_thr[0].ready_queue, (node_t *) &main_thread);
    if (swapcontext(&(main_thread.context), kernel_thr[0].context) == -1) { // SWAP TO THE THREAD FROM THE QUEUE
        handle_error("swapcontext");
    }

    return 0;
}

thread_t *thread_create(void (body)(void *), void *arg, int deps, thread_t *successors[]) {

    thread_t *thr;

#ifdef REUSE_STACK
    int empty_descriptors = 1;
    thr = (thread_t *) dequeue_tail(thr_reuse.descriptors);
    if (thr != NULL) {
    	empty_descriptors = 0;
        // thr_reuse.capacity--; 
        __sync_fetch_and_add(&(thr_reuse.capacity), -1);
    }
    else {
#endif
    thr = (thread_t *) malloc(sizeof(thread_t));
    if (getcontext(&(thr->context)) == -1) {
        handle_error("getcontext");
    }
    thr->stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);    // ALLOCATE ALIGNED MEMORY 8*MEM_SIZE
    thr->successors = NULL;
    memcpy(thr->stack, &thr, sizeof(thread_t *));                   // FIRST 16 BYTES ARE FOR THE POINTER POINTING TO THE DESCRIPTOR
    (thr->context).uc_stack.ss_sp = thr->stack + 16;                // ALL THE OTHER BYTES
    (thr->context).uc_stack.ss_size = STACK_SIZE - 16;
    (thr->context).uc_link = NULL;
    thr->alloc_successors = 0;
#ifdef REUSE_STACK
    }
#endif
    
    makecontext(&(thr->context), (void *)wrapper_func, 2, body, arg);
    thr->next = NULL;
    thr->prev = NULL;

    __sync_fetch_and_add(&no_threads, 1);
    thr->id = __sync_fetch_and_add(&thread_next_id, 1);
    //("THREAD CREATE %d\n", thr->id);
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

    // thr->kernel_thread_id = thread_self()->kernel_thread_id;
    if (!thr->deps) {
        enqueue_tail(kernel_thr[thread_self()->kernel_thread_id].ready_queue, (node_t *) thr);
        __sync_fetch_and_add(&(kernel_thr[thread_self()->kernel_thread_id].num_threads), 1);
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
    self = *(thread_t **) (mask & ((long int)&mask));       // get address of a variable and find begining of stack

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
    __sync_fetch_and_add(&(me->deps), num_deps+1);                // ????
    return 0;
}

int thread_yield() {
    thread_t *me;
    
    me = thread_self();
    // me->old_deps = me->deps;                                     //!!!!!!
    //("THREAD YIELD: yield from thread %d\n", me->id);
    fflush(stdout);
    
    // TODO mentio to CDA 
    if (swapcontext(&(me->context), kernel_thr[me->kernel_thread_id].context) == -1) {
        handle_error("swapcontext");
    }
    return 0;
}

void thread_exit() {
    thread_t *me;


    me = thread_self();
    //("THREAD EXIT %d\n",me->id);
    fflush(stdout);
    if(me->alive == 1) {
        for (int i = 0; i < me->num_successors; i++) {
            int curr_deps = __sync_fetch_and_add(&(me->successors[i]->deps), -1);
            if(curr_deps == 1 && !me->successors[i]->blocked) {
                // printf("THREAD EXIT: thread %d successor %d has 0 deps, adding him in the queue\n", me->id, me->successors[i]->id);
                //flush(stdout);
                enqueue_tail(kernel_thr[me->kernel_thread_id].ready_queue, (node_t *) me->successors[i]);
                __sync_fetch_and_add(&(kernel_thr[me->kernel_thread_id].num_threads), 1);

            }
        }
    }
    me->alive = 0;
    if (me->id) {
    	// print_queue(ready_queue);
        if (swapcontext(&(me->context), kernel_thr[me->kernel_thread_id].context) == -1) {
            handle_error("swapcontext");
        }
    }
}

int thread_lib_exit() {
    int status;

    terminate = 1;
    printf("LIB EXITTTTT %d, %d and %d\n", thread_self()->kernel_thread_id, thread_self()->id, no_threads);

#ifdef REUSE_STACK
    thread_t *thr;

    thr = (thread_t *) dequeue_head(thr_reuse.descriptors);
    while (thr != NULL) {
        (thr_reuse.capacity)--;
        // __sync_fetch_and_add(&(thr_reuse.capacity), -1);
        if (thr->num_successors > 0) {
            free(thr->successors);
        }
        free(thr->stack);
        free(thr);
        thr = (thread_t *) dequeue_head(thr_reuse.descriptors);
    }
#endif
    // free(main_thread.successors);
    // free(uctx_scheduler.uc_stack.ss_sp);
    
    // for (int i = 0; i < no_native_threads; i++) {
    //     free(kernel_thr[i].ready_queue);
    //     free(kernel_thr[i].context);
    //     free(kernel_thr[i].stack);
    // }
    // free(kernel_thr); 

    for (int i = 1; i < no_native_threads; i++) {
        pid_t pid = waitpid(kernel_thr[i].pid, &status, 0);
        printf("Process %d is terminating..\n", pid);
    }
    return 0;
}

void free_thread(thread_t *thr) {

#ifdef REUSE_STACK
    if (thr->id) {
        enqueue_tail(thr_reuse.descriptors, (node_t *) thr);
        // thr_reuse.capacity++;
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
    thread_t *thr;
    int k, i, curr_num_threads;

    if (no_native_threads == 1) {
        //printf("1 WORK STEALING: no native threads, from %d\n", native_thread);
        return;
    }

    for (k = 0, i = ((native_thread + 1) % no_native_threads); k < (no_native_threads - 1); k++, i = ((i + 1) % no_native_threads)) {
        curr_num_threads = kernel_thr[i].num_threads;

        if (curr_num_threads > 2) {
            // printf("curr_num_threads 2 from native %d is %d\n", i, curr_num_threads);
            thr = (thread_t *) dequeue_head(kernel_thr[i].ready_queue);
            if (thr == NULL) {
                printf("here null\n");
                fflush(stdout);
                continue;
            }
            if (thr->id == 0) {
                printf("HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE 0\n");
                fflush(stdout);
                enqueue_head(kernel_thr[0].ready_queue, (node_t *) thr);
                continue;
            }
            enqueue_head(kernel_thr[native_thread].ready_queue, (node_t *) thr);

            __sync_fetch_and_add(&(kernel_thr[i].num_threads), -1);
            __sync_fetch_and_add(&(kernel_thr[native_thread].num_threads), 1);

            // printf("Native thread %d, GOOOOOT thread %d from %d\n", native_thread, thr->id, i);
            // fflush(stdout);
            // printf("and ...curr_num_threads 2 from native %d is %d\n", i, res);
            break;
        }
    }
}

void scheduler(void *id) {
    thread_t *running_thread;
    int native_thread = *((int *)id);
    int temp_deps;

    while(!terminate) {
        running_thread = (thread_t *) dequeue_tail(kernel_thr[native_thread].ready_queue);
        if (running_thread == NULL) {
            work_stealing(native_thread);
            continue;
        }
        running_thread->kernel_thread_id = native_thread;
        __sync_fetch_and_add(&(kernel_thr[native_thread].num_threads), -1);
        // print_queue(ready_queue);
        printf("run thr %d, kernel thr %d\n", running_thread->id, native_thread);
        fflush(stdout);

        // running_thread->context.uc_link = kernel_thr[native_thread].context;
        if (swapcontext(kernel_thr[native_thread].context, &(running_thread->context)) == -1) {
            handle_error("swapcontext");
        }

        printf("return thr %d, kernel thr %d\n", running_thread->id, native_thread);
        fflush(stdout);
        if (running_thread->alive == 0) {
            free_thread(running_thread);
            __sync_fetch_and_add(&no_threads, -1);
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
                // printf("HEREEEEEEE %d with old_deps = %d\n", running_thread->id, running_thread->old_deps);
                enqueue_head(kernel_thr[native_thread].ready_queue, (node_t *) running_thread);
                __sync_fetch_and_add(&(kernel_thr[running_thread->kernel_thread_id].num_threads), 1);
            }
        }
        // Push myself to queue from yield so we are protected if another native thread take us before we swapcontex
    }
}

void wrapper_func(void (body)(void *), void *arg) {
    body(arg);
    thread_exit();
}

void print_queue(queue_t *queue) {
    node_t *curr;
    thread_t *thr;

    lock_acquire(&(queue->lock));
    printf("~~~~~\n");
    for (curr = queue->head->next; curr != queue->head; curr = curr->next) {
        thr = (thread_t *)curr;
        printf("%d ", thr->id);
    }
    printf("\n~~~~~\n");
    lock_release(&(queue->lock));
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
    
    array_of_successors = (thread_t **) malloc((nargs + 1)*sizeof(thread_t *));
    array_of_successors[nargs] = NULL;

    va_start(ap, nargs);
    for (int i = 0; i < nargs; i++) {
        array_of_successors[i] = va_arg(ap, thread_t *);
    }
    va_end(ap);
    return array_of_successors;
}
