#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>

// These are some constants for our library
typedef unsigned long addr;     // refers to addresses in our program
#define JB_SP 6                 // refers to the stack pointer register in the jbuf struct
#define JB_PC 7                 // refers to the program counter register in the jbuf struct
#define STACK_SIZE 4096         // in bytes
#define MAX_Q_ITEMS 20	        // max length of the queue


// this is the period for the timer
// the timer is called after this much time has elapsed
#define TIME_SLICE 100000; // in microseconds

#define SHOW_ERROR 1

#define DEBUG(...)\
if(SHOW_ERROR)\
{\
	printf("\033[0;31m [Debug message]: ");\
	printf("%s, %d ", __FUNCTION__, __LINE__);\
	printf(__VA_ARGS__);\
	printf("\033[0m");\
}

// all the states our threads can assume
enum ThreadStates {
	READY,
	BLOCKED,
	RUNNING,
	FINISHED
};


// the struct for a single thread
// this represents the Thread Control Block
struct TCB {
    //Stack info 
	char* stack;
	int stack_size;
	addr sp;	// stack pointer
	addr pc;    // program counter

	sigjmp_buf jbuf;    // Context
	
	int thread_id;
	int priority;
	enum ThreadStates state;

	void (*callback_routine);

	//Thread id of the thread waiting on this thread, in case a thread calls join on this thread
	int waiting_id;
};


// Populate this struct with the properties required
// when implementing task 6
struct Semaphore {
};


// initialize the library and its variables (if any, like the queues)
// its use depends on your implementation
void init_lib();

// returns a timestamp in milliseconds. use to calculate time intervals
int get_time();

// these functions are used to disable and enable the clock signals
// there are places in the code where we cannot allow context switch to happen
// these will be useful there
void enable_interrupts();
void disable_interrupts();

// a black box function to translate addresses when accessing system registers directly
// (IGNORE)
addr map_address(addr address);

// handles the alarm clock singal
// so essentially it runs periodically and schedules a new thread after a time slice is complete
void scheduler(int signal_number);

int create_thread(void (*callback));
void end_thread();
void switch_context(struct TCB* old_thread, struct TCB* new_thread);


int self_id();
void sleep(int milliseconds);
void yield();
void join(int thread_id);
void block();
void sem_init(struct Semaphore* sem, int value);
void sem_wait(struct Semaphore* sem);
void sem_post(struct Semaphore* sem);


// for the alarm interrupt
void timer_start();
void timer_stop();
