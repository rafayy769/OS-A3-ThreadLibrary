#include "thread.h"

void worker() {
	int id = self_id();

	while (1) {
		for (int i = 0; i < 1000000; i++);
		printf("Thread running: %d\n", id);
	}

	end_thread();
}

int main() {
	init_lib();
	int main_thread = create_thread(NULL);

	// Creating 4 threads. Each thread will print its id in an infinite loop
	// The scheduler will keep on switching threads, so the output should keep changing
	// The output will run infinitely.
	create_thread(worker);
	create_thread(worker);
	create_thread(worker);
	create_thread(worker);


	timer_start();
	while(1){
	}
}