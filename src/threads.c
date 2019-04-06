#include "threads.h"

int create_kernel_thread(kernel_thread_t *kernel_thr) {

    // // printf("ALLOC KERNEL THREAD STACK\n");
    fflush(stdout);
    char *stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);
    kernel_thr->stack = stack + STACK_SIZE;
    return clone(&wrapper_scheduler, kernel_thr->stack, CLONE_VM, (void *)&kernel_thr->pid);       //CLONE_VM
}

int wrapper_scheduler(void *id) {
    // // printf("HEREEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
    fflush(stdout);
    scheduler(id);
    return 1;
}

int thread_lib_init(int native_threads) {

    struct rlimit current_limits;
    long int mask;
    char *native_thread_stack;
    static ucontext_t uctx_main;

    // printf("LIB INIT\n");
    fflush(stdout);
#ifdef REUSE_STACK
    thr_reuse.descriptors = queue_create();
    thr_reuse.capacity = 0;
    thr_reuse.max_capacity = 10000;
#endif

    thread_next_id = 0;
    ready_queue = queue_create();
    //pthread_mutex_init(&lock_queue, NULL);
    //pthread_mutex_init(&lock_stack_queue, NULL);

    main_thread.id = thread_next_id++;
    main_thread.deps = 0;
    main_thread.alive = 1;
    main_thread.blocked = 0;
    main_thread.next = NULL;
    main_thread.prev = NULL;
    main_thread.stack = NULL;
    main_thread.successors = (thread_t **) malloc(sizeof(thread_t *));
    main_thread.successors[0] = NULL;
    main_thread.context = uctx_main;
    main_thread.kernel_thread_id = 0;

    getrlimit(RLIMIT_STACK, &current_limits);
    native_stack_size = current_limits.rlim_cur;
    // printf("NATIVE THREAD WITH STACK SIZE = %ld\n", native_stack_size);
    
    mask = ~(native_stack_size - 1);
    native_thread_stack = (char *) (mask & ((long int)&mask));       // get address of a variable and find begining of stack
    main_thread.context.uc_stack.ss_sp = native_thread_stack;

    //pthread_mutex_lock(&lock_queue);
    enqueue_head(ready_queue, (queue_t *) &main_thread);
    // print_queue(ready_queue);
    //pthread_mutex_unlock(&lock_queue);
    /********************************************************************/

    kernel_thr = (kernel_thread_t *) malloc(native_threads*sizeof(kernel_thread_t));    // REMEMBER TO FREE
    kernel_thr[0].pid = 0;                                                             // SAVE MAIN ID, i.e 0 id
    kernel_thr[0].stack = native_thread_stack;                                         // FOR COMPLECITY
    kernel_thr[0].context = /*****/ &uctx_scheduler; /*****/                      // FOR COMPLECITY 

    for (int i = 0; i < (native_threads - 1); i++) {         // native_threads - 1, cause main has already an id
        // printf("GOING TO CREATE NATIVE THREAD\n");
        kernel_thr[i + 1].pid = i + 1;
        kernel_thr[i + 1].context = (ucontext_t *) malloc(sizeof(ucontext_t));  // REMEMBER TO FREE
        printf("ON CREATE: %d - %p\n",i+1, kernel_thr[i + 1].context);
        create_kernel_thread(&kernel_thr[i + 1]);
    }
    fflush(stdout);
    /********************************************************************/

    if (getcontext(&uctx_scheduler) == -1) {
        handle_error("getcontext");
    }
    char *scheduler_stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);     // ALLOCATE ALIGNED MEMORY 8*MEM_SIZE
    uctx_scheduler.uc_stack.ss_sp = scheduler_stack;                            // ALL THE OTHER BYTES
    uctx_scheduler.uc_stack.ss_size = STACK_SIZE;
    makecontext(&(uctx_scheduler), (void *)scheduler, 1, (void *) &kernel_thr[0].pid);
    // printf("PRE swapcontext");
    if (swapcontext(&(main_thread.context), kernel_thr[0].context) == -1) { // SWAP TO THE THREAD FROM THE QUEUE
        handle_error("swapcontext");
    }

    return 0;
}

thread_t *thread_create(void (body)(void *), void *arg, int deps, thread_t *successors[]) {

    thread_t *thr;

#ifdef REUSE_STACK
    //pthread_mutex_lock(&lock_stack_queue);
    int empty_descriptors = queue_empty(thr_reuse.descriptors);
    //pthread_mutex_unlock(&lock_stack_queue);
    if (!empty_descriptors) {
        // printf("THREAD CREATE: got reuse thread descriptor\n");
        //pthread_mutex_lock(&lock_stack_queue);
        thr = (thread_t *) dequeue_tail(thr_reuse.descriptors);
        //pthread_mutex_unlock(&lock_stack_queue);
        thr_reuse.capacity--; 
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
    (thr->context).uc_link = &uctx_scheduler;
    thr->alloc_successors = 0;
#ifdef REUSE_STACK
    }
#endif
    
    makecontext(&(thr->context), (void *)wrapper_func, 2, body, arg);
    thr->next = NULL;
    thr->prev = NULL;
    thr->id = thread_next_id++;
    thr->deps = deps;
    thr->alive = 1;
    thr->blocked = 0;
    thr->num_successors = 0;
    
    // printf("THREAD CREATE: stack address %p of thread %d\n", thr->stack, thr->id);
    fflush(stdout);

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
    
    // printf("THREAD CREATE: deps %d, num_successors %d, alloc_successors %d of thread %d\n", thr->deps, thr->num_successors, thr->alloc_successors, thr->id);
    fflush(stdout);
    for (int i = 0; i < thr->num_successors; i++) {
        thr->successors[i] = successors[i];
    }

    if (!thr->deps) {
        //pthread_mutex_lock(&lock_queue);
        enqueue_tail(ready_queue, (queue_t *) thr);
        // print_queue(ready_queue);
        //pthread_mutex_unlock(&lock_queue);
    }
    return thr;
}

thread_t *thread_self() {
    long int mask;
    thread_t *self;
    char *stack_pointer;

    mask = ~(native_stack_size - 1);
    stack_pointer = (char *) (mask & ((long int)&mask));
    // printf("SELF\n");
    fflush(stdout);
    if (stack_pointer == main_thread.context.uc_stack.ss_sp) {
        return &main_thread;
    }

    mask = ~(STACK_SIZE - 1);
    self = *(thread_t **) (mask & ((long int)&mask));       // get address of a variable and find begining of stack
    // // printf("THREAD SELF: self pointer %p of thread %d\n", self, self->id);
    fflush(stdout);
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
    // printf("THREAD INC DEPENDENCY: increment dep %d of thread %d\n", num_deps, me->id);
    fflush(stdout);
    __atomic_add_fetch( &(me->deps), num_deps, __ATOMIC_SEQ_CST);
    return 0;
}

int thread_yield() {
    thread_t *me;
    
    me = thread_self();
    // printf("THREAD YIELD: yield from thread %d\n", me->id);
    fflush(stdout);
    if (!me->deps && !me->blocked) {
            // printf("THREAD YIELD: push myself to queue from thead %d\n", me->id);
            fflush(stdout);
            //pthread_mutex_lock(&lock_queue);
            enqueue_head(ready_queue, (queue_t *) me);
            // print_queue(ready_queue);
            //pthread_mutex_unlock(&lock_queue);
    }
    // printf("YEILDDDDDDDDDDDDd\n");
    // printf("%d\n", kernel_thr[me->kernel_thread_id].pid);
    fflush(stdout);
    if (swapcontext(&(me->context), kernel_thr[me->kernel_thread_id].context) == -1) {
        handle_error("swapcontext");
    }
    return 0;
}

void thread_exit() {
    thread_t *me;

    // printf("THREAD EXIT\n");
    fflush(stdout);

    me = thread_self();
    if(me->alive == 1) {
        (me->context).uc_link = kernel_thr[me->kernel_thread_id].context;
        // printf("THREAD EXIT: %p, %d \n", kernel_thr[me->kernel_thread_id].context, me->kernel_thread_id);
        // fflush(stdout);
        // printf("THREAD EXIT: exiting thread %d from native thread %d \n", me->id, me->kernel_thread_id);
        // printf("THREAD EXIT: thread %d successors are %d\n", me->id, me->num_successors);
        fflush(stdout);
        for (int i = 0; i < me->num_successors; i++) {
            // printf("THREAD EXIT: thread %d descent the deps %d from thread %d\n", me->id, me->successors[i]->deps, me->successors[i]->id);
            fflush(stdout);
            __atomic_sub_fetch(&(me->successors[i]->deps), 1, __ATOMIC_SEQ_CST);
            // me->successors[i]->deps--;
            if(me->successors[i]->deps == 0 && !me->successors[i]->blocked) {
                // printf("THREAD EXIT: thread %d successor %d has 0 deps, adding him in the queue\n", me->id, me->successors[i]->id);
                fflush(stdout);
                //pthread_mutex_lock(&lock_queue);
                enqueue_tail(ready_queue, (queue_t *) me->successors[i]);
                // print_queue(ready_queue);
                //pthread_mutex_unlock(&lock_queue);
            }
        }
    }
    me->alive = 0;
    // if (swapcontext(&(me->context), kernel_thr[me->kernel_thread_id].context) == -1) {
    //     handle_error("swapcontext");
    // }
}

int thread_lib_exit() {
    
    // printf("THREAD LIB EXIT: exit library\n");
    fflush(stdout);

#ifdef REUSE_STACK
    thread_t *thr;
    while (!queue_empty(thr_reuse.descriptors)){
        thr = (thread_t *) dequeue_head(thr_reuse.descriptors);
        (thr_reuse.capacity)--;
        if (thr->num_successors > 0) {
            free(thr->successors);
        }
        free(thr->stack);
        free(thr);
    }
#endif
    free(main_thread.successors);
    free(ready_queue);
    free(uctx_scheduler.uc_stack.ss_sp);
    return 0;
}

void free_thread(thread_t *thr) {

#ifdef REUSE_STACK
    if (thr->id /*&& thr_reuse.capacity < thr_reuse.max_capacity*/) {
        // printf("FREE_THREAD: will reuse the stack of thread %d, descriptors queue has size %d\n", thr->id, thr_reuse.capacity);
        //pthread_mutex_lock(&lock_stack_queue);
        enqueue_tail(thr_reuse.descriptors, (queue_t *) thr);
        thr_reuse.capacity++;
        //pthread_mutex_unlock(&lock_stack_queue);
        // printf("~~~~~~~~~~~~~`\n");
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

void scheduler(void *id) {
    thread_t *running_thread;
    int native_thread = *((int *)id);

    while(1) {
        //pthread_mutex_lock(&lock_queue);
        running_thread = (thread_t *) dequeue_tail(ready_queue);
        //pthread_mutex_unlock(&lock_queue);
        // // printf("SCHEDULER: %p from kernel thread %d\n", running_thread, native_thread);
        // fflush(stdout);
        if (running_thread == NULL) {
            continue;
        }
        // print_queue(ready_queue);

        printf("SCHEDULER: run the next thread %d from kernel thread %d\n", running_thread->id, native_thread);
        fflush(stdout);
        sleep(1);

        running_thread->kernel_thread_id = native_thread;
        if (swapcontext(kernel_thr[native_thread].context, &(running_thread->context)) == -1) {
            handle_error("swapcontext");
        }

        printf("SCHEDULER: return of thread thread %d on kernel thread %d\n", running_thread->id, native_thread);
        fflush(stdout);
        if (running_thread->alive == 0) {
            free_thread(running_thread);
        }
    }
}

void wrapper_func(void (body)(void *), void *arg) {
    body(arg);
    thread_exit();
}

thread_t **THREAD_LIST(thread_t *successor){
    thread_t **array_of_successors;
    
    array_of_successors = (thread_t **) malloc(2*sizeof(thread_t *));
    array_of_successors[0] = successor;
    array_of_successors[1] = NULL;
    return array_of_successors;
}

thread_t **THREAD_LIST2(int nargs, ...){
    thread_t **array_of_successors;
    va_list ap;
    
    array_of_successors = (thread_t **) malloc((nargs+1)*sizeof(thread_t *));
    array_of_successors[nargs] = NULL;

    va_start(ap, nargs);
    for(int i = 0; i < nargs; i++) {
        array_of_successors[i] = va_arg(ap, thread_t *);
    }
    va_end(ap);
    return array_of_successors;
}