#include "thread.h"

// mask used to ignore some signals
// we need to ignore the alarm clock signal when we are doing critical stuff
// an empty masks means all signals are enabled
sigset_t signal_mask;

// the timer for the alarm clock
struct itimerval timer;

// stores the pointer to the currently running thread
struct TCB* running = NULL;
int num_threads = 0;

addr map_address(addr address) {
	addr ret;
	asm volatile("xor    %%fs:0x30,%0\n"
			"rol    $0x11,%0\n"
			: "=g" (ret)
			  : "0" (address));
	return ret;
}

void timer_start() {
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = TIME_SLICE;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = TIME_SLICE;

    // when a time slice is over, call the scheduler function
    // by raising the SIGALRM signal
    signal(SIGALRM, scheduler);

    // this mask will be used to mask the alarm signal when we are
    // performing critical tasks that cannot be left midway
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGALRM); // add only the alarm signal

    // start the timer
    setitimer(ITIMER_REAL, &timer, NULL);
}

void timer_stop() {
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL);
}

void enable_interrupts() {
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
}

void disable_interrupts() {
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
}

/**
 * @brief Creates a new queue
 *
 * @return queue* Pointer to the newly created queue
 */
queue *init_queue()
{
    queue *q = (queue *)malloc(sizeof(queue));
    for (int i = 0; i < MAX_Q_ITEMS; i++)
    {
        q->threads[i] = NULL;
    }
    q->curr_size = 0;
    return q;
}

/**
 * @brief Enqueues a TCB into the queue.
 *
 * @param q The queue to enqueue into.
 * @param tcb The TCB to enqueue.
 * @return void
 */
void enqueue(queue *q, struct TCB *tcb)
{
    if (q->curr_size == MAX_Q_ITEMS)
    {
        printf("Queue is full\n");
        return;
    }
    q->threads[q->curr_size++] = tcb;
}

/**
 * @brief Dequeue a thread from the queue.
 *
 * @param q The queue to dequeue from.
 * @return struct TCB* The dequeued thread.
 */
struct TCB *dequeue(queue *q)
{
    if (q->curr_size == 0)
    {
        printf("Queue is empty\n");
        return NULL;
    }
    struct TCB *tcb = q->threads[0];
    for (int i = 0; i < q->curr_size - 1; i++)
    {
        q->threads[i] = q->threads[i + 1];
    }
    q->curr_size--;
    return tcb;
}

// print queue
void print_queue(queue *q)
{
    if (SHOW_ERROR_MSGS)
    {
        printf("Queue: ");
        for (int i = 0; i < q->curr_size; i++)
        {
            printf("%d ", q->threads[i]->thread_id);
        }
        printf("\n");
    }
}

// remove an element from the queue
// returns the element if the element was found and removed
// returns the NULL if the element was not found
struct TCB *remove_from_queue(queue *q, int tid)
{
    for (int i = 0; i < q->curr_size; i++)
    {
        if (q->threads[i]->thread_id == tid)
        {
            struct TCB *tcb = q->threads[i];
            for (int j = i; j < q->curr_size - 1; j++)
            {
                q->threads[j] = q->threads[j + 1];
            }
            q->curr_size--;
            return tcb;
        }
    }
    return NULL;
}

// search for a thread in the priority queue with the thread id
// returns the thread if found
// returns NULL if not found
struct TCB* search_priority_queue(queue* q, int tid) {
    for (int i = 0; i < q->curr_size; i++) {
        if (q->threads[i]->thread_id == tid) {
            return q->threads[i];
        }
    }
    return NULL;
}

// goes through the queue and removes and returns the thread with the maximum priority. Rearranges things around.
struct TCB* dequeue_with_priority(queue* q)
{
    if (q->curr_size == 0)
    {
        return NULL;
    }
    struct TCB* max = q->threads[0];
    int max_index = 0;
    for (int i = 1; i < q->curr_size; i++)
    {
        if (q->threads[i]->priority > max->priority)
        {
            max = q->threads[i];
            max_index = i;
        }
    }
    for (int i = max_index; i < q->curr_size - 1; i++)
    {
        q->threads[i] = q->threads[i + 1];
    }
    q->curr_size--;
    for (int i = 0; i < q->curr_size; i++)
    {
        q->threads[i]->priority++;
    }
    return max;
}

// Global queues for ready and finished threads
queue* ready_queue;
queue* finished_queue;
queue* blocked_queue;

void init_lib() {
    ready_queue = init_queue();
    finished_queue = init_queue();
    blocked_queue = init_queue();
}

// returns id of the current thread
int self_id() {
    return running->thread_id;
}

int get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);

    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

//--------------------------------------------------------------------------------------------------------------------------
// manages the threads and decides on which thread to run next
void scheduler(int signal_number) {
    DEBUG("scheduler called \n");
    DEBUG("running thread id: %d \n", running->thread_id);

    // if the running thread is not finished, enqueue it
    if (running->state != FINISHED && running->state != BLOCKED) {
        DEBUG("enqueuing running thread \n");
        running->state = READY;
        enqueue(ready_queue, running);
    }

    // if there are no more threads to run, exit
    if (ready_queue->curr_size == 0) {
        DEBUG("no more threads to run \n");
        enable_interrupts();
        exit(0);
    }

    // get the next thread to run
    struct TCB* next_thread = dequeue_with_priority(ready_queue);
    DEBUG("next thread id: %d \n", next_thread->thread_id);
    struct TCB* prev_thread = running;
    running = next_thread;

    prev_thread->state = READY;
    next_thread->state = RUNNING;

    // switch to the next thread
    switch_context(prev_thread, next_thread);    
}

int create_thread(void (*callback), int priority) {
    
    struct TCB* thread = (struct TCB*) malloc(sizeof(struct TCB));

	//Thread ID
	thread->thread_id = ++num_threads;

    // Allocating the stack
    thread->stack = (char*) malloc(STACK_SIZE);
    thread->stack_size = STACK_SIZE;
    thread->sp = (addr)thread->stack + STACK_SIZE - sizeof(int); 

    // setting the Program Counter to the thread callback_routine
    thread->pc = (addr) callback;
    thread->callback_routine = callback;
    thread->waiting_id = -1;    // no thread is waiting on it. used in the case of the "join" function

    // setting the priority
    thread->priority = priority;

    if (num_threads == 1) {
        // this means we are creating the first thread, that is the main thread
        running = thread;
        running->state = RUNNING;
        DEBUG("created main thread with id: %d \n", running->thread_id);
    }
    else {
        // set the PC and SP registers to the address of the function we passed
        sigsetjmp(thread->jbuf, 1);
        (thread->jbuf->__jmpbuf)[JB_SP] = map_address(thread->sp);
        (thread->jbuf->__jmpbuf)[JB_PC] = map_address(thread->pc);
        sigemptyset(&thread->jbuf->__saved_mask);

        thread->state=READY;
        
        /*
        Task1: Add this thread to the ready queue
        */
        enqueue(ready_queue, thread);
        DEBUG("created thread with id: %d \n", thread->thread_id);
    }
       
	return thread->thread_id;
}

void end_thread() {

    DEBUG("ending thread with id: %d \n", running->thread_id);

    disable_interrupts();
    running->state = FINISHED;
    --num_threads;

    /*
    Task1: Add this thread to the finished_queue
    */
    enqueue(finished_queue, running);

    if(running->waiting_id != -1) {
        struct TCB* waiting_thread = search_priority_queue(blocked_queue, running->waiting_id);
        if (waiting_thread != NULL) {
            DEBUG("unblocking thread with id: %d \n", waiting_thread->thread_id);
            waiting_thread->waiting_id = -1;
            waiting_thread->state = READY;
            enqueue(ready_queue, waiting_thread);
        }
    }

    // passing -1 as it is not being called as a result of an interrupt
    // and we are calling it manually
    scheduler(-1);
}

void switch_context(struct TCB* old_thread, struct TCB* new_thread) {
    DEBUG("switching context from %d to %d\n", old_thread->thread_id, new_thread->thread_id);
    // reset the timer for the new process that is going to run
    // so that i receives its full time slice
    setitimer(ITIMER_REAL, &timer, NULL);

    // save the context of the current running thread
    int ret_val = sigsetjmp(old_thread->jbuf, 1);
    if (ret_val == 1) {
        enable_interrupts();
        return;
    }

    enable_interrupts();
    // switch to the next thread
    siglongjmp(new_thread->jbuf, 1);
}

void yield()
{
    DEBUG("yield called \n");
    disable_interrupts();
    scheduler(0);
}

void sleep(int ms) 
{
    int start_time = get_time();
    while (get_time() - start_time < ms) {
        // busy-wait
    }
}

// implement the block function
void block()
{
    running->state = BLOCKED;
    enqueue(blocked_queue, running);
    scheduler(0);
}

// implement the join function. When a thread calls join on another thread, it blocks itself and stores its thread_id in the waiting_id property of the thread it is waiting on. When the other thread ends, it unblocks the thread waiting on it.
void join(int thread_id)
{
    // search for the thread in the ready queue.
    // if found, set the waiting_id property of the thread to the id of the current thread
    // and block the current thread

    DEBUG("join called by thread with id: %d on %d.\n", running->thread_id, thread_id);

    struct TCB* thread = search_priority_queue(ready_queue, thread_id);
    if (thread != NULL) {
        DEBUG("thread with id: %d is waiting on thread with id: %d \n", running->thread_id, thread_id);
        thread->waiting_id = running->thread_id;
        block();
    }
}


// --------------------------------
// Semaphore functions
// --------------------------------

// initialize the semaphore
void sem_init(struct Semaphore* sem, int value) {
    sem->value = value;
    sem->waiting_queue = init_queue();
}

// implement the wait function
void sem_wait(struct Semaphore* sem)
{
    sem->value--;
    if (sem->value < 0) 
    {
        running->state = BLOCKED;
        enqueue(sem->waiting_queue, running);
        scheduler(0);
    }
}

// implement the post function
void sem_post(struct Semaphore* sem) 
{
    sem->value++;
    if (sem->value <= 0) {
        struct TCB* thread = dequeue(sem->waiting_queue);
        thread->state = READY;
        enqueue(ready_queue, thread);
        scheduler(0);
    }
}