#include "thread.h"

// Expected output:

// Thread: 3, Counter: 0
// Thread: 4, Counter: 0
// Thread: 5, Counter: 0
// Thread: 3, Counter: 1
// Thread: 4, Counter: 1
// Thread: 5, Counter: 1
// Thread: 3, Counter: 2
// Thread: 4, Counter: 2
// Thread: 5, Counter: 2
// Thread: 3, Counter: 3
// Thread: 4, Counter: 3
// Thread: 5, Counter: 3
// Thread: 3, Counter: 4
// Thread: 4, Counter: 4
// Thread: 5, Counter: 4
// Thread: 2, Counter: 0
// Thread: 2, Counter: 1
// Thread: 2, Counter: 2
// Thread: 2, Counter: 3
// Thread: 2, Counter: 4

void worker() {
	int id = self_id();

	for (int i = 0; i < 5; i++) {
		printf("Thread: %d, Counter: %d\n", id, i);
		yield();
	}

	end_thread();
}

void worker_function2() {
	int id = self_id();
	
	// 1 second sleep
	sleep(1000);

	for (int i = 0; i < 5; i++) {
		printf("Thread: %d, Counter: %d\n", id, i);
	}

	end_thread();
}

int main() {
	init_lib();
	int main_thread = create_thread(NULL);

	create_thread(worker_function2);
	create_thread(worker);
	create_thread(worker);
	create_thread(worker);


	timer_start();
	while(1){
	}
}