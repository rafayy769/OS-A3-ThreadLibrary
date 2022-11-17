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

// Queue node, contains a pointer to next and a pointer to the TCB
typedef struct QNode {
    struct QNode* next;
    struct TCB* tcb;
} node;

// Queue data structure, maintain a tail pointer as well as a head pointer for faster enqueue and removals
typedef struct Queue {
    node* head;
    node* tail;
    int size;
} queue;

/**
 * @brief Creates a new queue
 * 
 * @return queue* Pointer to the newly created queue
 */
queue* init_queue() {
    queue* q = (queue*) malloc(sizeof(queue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

/**
 * @brief Enqueues a TCB into the queue.
 * 
 * @param q The queue to enqueue into.
 * @param tcb The TCB to enqueue.
 * @return void
 */
void enqueue(queue* q, struct TCB* tcb)
{
    node* new_node = (node*) malloc(sizeof(node));
    new_node->tcb = tcb;
    new_node->next = NULL;
    if (q->size == 0) 
    {
        q->head = new_node;
        q->tail = new_node;
    } 
    else 
    {
        q->tail->next = new_node;
        q->tail = new_node;
    }
    q->size++;
}

/**
 * @brief Dequeue a thread from the queue.
 * 
 * @param q The queue to dequeue from.
 * @return struct TCB* The dequeued thread.
 */
struct TCB* dequeue(queue* q)
{
    if (q->size == 0) 
    {
        return NULL;
    }

    node* temp = q->head;
    q->head = q->head->next;
    q->size--;
    struct TCB* tcb = temp->tcb;
    free(temp);
    return tcb;
}

void print_queue(queue* q) {
    node* temp = q->head;
    while (temp != NULL) {
        printf("%d ", temp->tcb->thread_id);
        temp = temp->next;
    }
    printf("\n");
}

// Global queues for ready and finished threads
queue* ready_queue;
queue* finished_queue;

void init_lib() {
    ready_queue = init_queue();
    finished_queue = init_queue();
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
    // print state of queues
    #ifdef SHOW_ERROR
    DEBUG("ready queue: ");
    print_queue(ready_queue);
    DEBUG("finished queue: ");
    print_queue(finished_queue);
    #endif

    // if the running thread is not finished, enqueue it
    if (running->state != FINISHED) {
        enqueue(ready_queue, running);
    }

    // if there are no more threads to run, exit
    if (ready_queue->size == 0) {
        exit(0);
    }

    // get the next thread to run
    struct TCB* next_thread = dequeue(ready_queue);
    DEBUG("next thread id: %d \n", next_thread->thread_id);
    struct TCB* prev_thread = running;
    running = next_thread;

    // if the next thread is finished, free its stack and call the scheduler again
    if (next_thread->state == FINISHED) {
        free(next_thread->stack);
        scheduler(0);
    }
    prev_thread->state = READY;
    next_thread->state = RUNNING;

    // switch to the next thread
    switch_context(prev_thread, next_thread);    
}

int create_thread(void (*callback)) {
    
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
    DEBUG("sleep called \n");
    int start_time = get_time();
    disable_interrupts();
    while (get_time() - start_time < ms) {
        // busy-wait
    }
    enable_interrupts();
}