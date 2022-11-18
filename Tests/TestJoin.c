#include "thread.h"


// Expected output:

// Thread: 6, Counter: 0
// Thread: 6, Counter: 1
// Thread: 6, Counter: 2
// Thread: 6, Counter: 3
// Thread: 6, Counter: 4
// Thread 2: My child ended, So i can run now
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
// Main thread: All other threads should've finished. Trying to join finished threads...


void worker() {
	int id = self_id();

	for (int i = 0; i < 5; i++) {
		printf("Thread: %d, Counter: %d\n", id, i);
		yield();
	}

	end_thread();
}

void t1_worker() {
	int mychild = create_thread(worker, 30);
	// waits for child to end;
	join(mychild);

	printf("Thread %d: My child ended, So i can run now\n", self_id());

	end_thread();
}

int main() {
	init_lib();
	int main_thread = create_thread(NULL, 1);

	int t1 = create_thread(t1_worker, 30);
	int t2 = create_thread(worker, 5);
	int t3 = create_thread(worker, 5);
	int t4 = create_thread(worker, 5);

	timer_start();
	
	yield();
	join(t1);
	
	sleep(2000);
	printf("Main thread: All other threads should've finished. Trying to join finished threads...\n");
	// sleep, while all other threads finish.
	// try to join a finished thread.
	join(t2);
	join(t3);
	join(t4);
}