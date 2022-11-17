#include "thread.h"

/*
THIS IS THE TEST FILE FOR TASK 7
*/


struct Semaphore writer_sem;
struct Semaphore reader_sem;
int c = 2;
int num_readers = 0;

void writer() {
	int id = self_id();

	sem_wait(&writer_sem);
	c *= 2;
	printf("Writer %d modified c to %d\n", id, c);
	printf("Writer %d going to yield\n", id);
	yield();
	printf("Writer %d back, after yield\n", id);
	sem_post(&writer_sem);

	end_thread();
}

void reader() {
	int id = self_id();

	// Reader acquire the lock before modifying numreader
    sem_wait(&reader_sem);
    num_readers++;
    if(num_readers == 1) {
        sem_wait(&writer_sem); // If this is the first reader, then it will block the writer
    }
    sem_post(&reader_sem);

    // Reading Section
    printf("Reader %d: read c as %d\n", id, c);
	yield();

    // Reader acquire the lock before modifying numreader
    sem_wait(&reader_sem);
    num_readers--;
    if(num_readers == 0) {
        sem_post(&writer_sem); // If this is the last reader, it will wake up the writer.
    }
    sem_post(&reader_sem);
	
	end_thread();
}

int main() {
	init_lib();
	create_thread(NULL, 1);

	sem_init(&writer_sem, 1);
	sem_init(&reader_sem, 1);
	int ids[20];
	for (int i = 0; i < 20; i+=2) {
		ids[i] = create_thread(writer, 1);
		ids[i+1] = create_thread(reader, 1);
	}

	timer_start();
	for(int i = 0; i < 20; i++)
		join(ids[i]);
}