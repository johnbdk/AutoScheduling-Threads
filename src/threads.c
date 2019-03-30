#include "threads.h"

int thread_lib_init(int native_threads) {

    struct rlimit current_limits;
    long int mask;
    char *native_thread_stack;
    static ucontext_t uctx_main;

    thread_next_id = 0;
    ready_queue = queue_create();

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

    getrlimit(RLIMIT_STACK, &current_limits);
    native_stack_size = current_limits.rlim_cur;
    printf("NATIVE THREAD WITH STACK SIZE = %ld\n", native_stack_size);
    
    mask = ~(native_stack_size - 1);
    native_thread_stack = (char *) (mask & ((long int)&mask));       // get address of a variable and find begining of stack
    main_thread.context.uc_stack.ss_sp = native_thread_stack;

    queue_push(ready_queue, (queue_t *) &main_thread);

    if (getcontext(&uctx_scheduler) == -1) {
        handle_error("getcontext");
    }
    char *scheduler_stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);     // ALLOCATE ALIGNED MEMORY 8*MEM_SIZE
    uctx_scheduler.uc_stack.ss_sp = scheduler_stack;                            // ALL THE OTHER BYTES
    uctx_scheduler.uc_stack.ss_size = STACK_SIZE;
    makecontext(&(uctx_scheduler), scheduler, 0);
        
    if (swapcontext(&(main_thread.context), &uctx_scheduler) == -1) {                       // SWAP TO THE THREAD FROM THE QUEUE
        handle_error("swapcontext");
    }
    return 0;
}

int thread_create(thread_t *thr, void (body)(void *), void *arg, int deps, thread_t *successors[]) {
    
    if (getcontext(&(thr->context)) == -1) {
        handle_error("getcontext");
    }
    thr->stack = (char *) aligned_alloc(STACK_SIZE, STACK_SIZE);    // ALLOCATE ALIGNED MEMORY 8*MEM_SIZE
    memcpy(thr->stack, &thr, sizeof(thread_t *));                   // FIRST 16 BYTES ARE FOR THE POINTER POINTING TO THE DESCRIPTOR
    (thr->context).uc_stack.ss_sp = thr->stack + 16;                // ALL THE OTHER BYTES
    (thr->context).uc_stack.ss_size = STACK_SIZE - 16;
    (thr->context).uc_link = &uctx_scheduler;
    makecontext(&(thr->context), (void *)wrapper_func, 2, body, arg);

    thr->next = NULL;
    thr->prev = NULL;
    thr->id = thread_next_id++;
    thr->deps = deps;
    thr->alive = 1;
    thr->blocked = 0;
    thr->num_successors = 0;
    thr->successors = NULL;
    
    printf("THREAD CREATE: stack address %p of thread %d\n", thr->stack, thr->id);
    fflush(stdout);

    if (successors[0] == NULL) {
        thr->successors = (thread_t **) malloc(sizeof(thread_t *));
        thr->successors[0] = NULL;
        return -1;
    }

    for (int j = 0; successors[j] != NULL; j++) {
        thr->num_successors++;
    }

    printf("THREAD CREATE: num_successors %d of thread %d\n", thr->num_successors, thr->id);
    fflush(stdout);
    thr->successors = (thread_t **) malloc((thr->num_successors+1)*sizeof(thread_t *));
    for (int i = 0; i < thr->num_successors; i++) {
        thr->successors[i] = successors[i];
    }
    /* NULL terminated array */
    thr->successors[thr->num_successors] = NULL;

    if (!thr->deps) {
        queue_push(ready_queue, (queue_t *) thr);
    }
    return 0;
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
    printf("THREAD SELF: self pointer %p of thread %d\n", self, self->id);
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
    printf("THREAD INC DEPENDENCY: increment dep %d of thread %d\n", me->deps, me->id);
    fflush(stdout);
    me->deps += num_deps;
    return 0;
}

int thread_yield() {
    thread_t *me;
    
    me = thread_self();
    printf("THREAD YIELD: yield from thread %d\n", me->id);
    fflush(stdout);
    if (!me->deps && !me->blocked) {
            printf("THREAD YIELD: push myself to queue from thead %d\n", me->id);
            fflush(stdout);
            queue_push(ready_queue, (queue_t *) me);
    }
    
    if (swapcontext(&(me->context), &uctx_scheduler) == -1) {
        handle_error("swapcontext");
    }
    return 0;
}

void thread_exit() {
    thread_t *me;

    me = thread_self();
    if(me->alive == 1) {
        printf("THREAD EXIT: exiting thread %d\n", me->id);
        printf("THREAD EXIT: my successors are %d\n", me->num_successors);
        fflush(stdout);
        if (me->successors[0] != NULL) {
            for (int i = 0; i < me->num_successors; i++) {
                printf("THREAD EXIT: descent the deps %d from thread %d\n", me->successors[i]->deps, me->successors[i]->id);
                fflush(stdout);
                me->successors[i]->deps--;
                if(me->successors[i]->deps == 0 && !me->successors[i]->blocked) {
                    printf("THREAD EXIT: successor %d has 0 deps, adding him in the queue\n", me->successors[i]->id);
                    fflush(stdout);
                    queue_push(ready_queue, (queue_t *) me->successors[i]);
                }
            }
        }
    }
    me->alive = 0;
}

int thread_lib_exit() {
    
    printf("THREAD LIB EXIT: exit library\n");
    fflush(stdout);
    free(main_thread.successors);
    free(ready_queue);
    free(uctx_scheduler.uc_stack.ss_sp);
    return 0;
}

void free_thread(thread_t *thr) {

    if (thr->num_successors > 0) {
        free(thr->successors);
    }
    if (thr->id) {
        free(thr->stack);
    }
}

void scheduler(void) {
    thread_t *running_thread;

    while(1) {
        running_thread = (thread_t *) queue_pop(ready_queue);
        if(running_thread == NULL){
            continue;
        }
        printf("SCHEDULER: queue is not empty, run the next thread %d\n", running_thread->id);
        fflush(stdout);

        if (swapcontext(&uctx_scheduler, &(running_thread->context)) == -1) {
            handle_error("swapcontext");
        }

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
