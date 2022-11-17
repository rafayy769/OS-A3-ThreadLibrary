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
// Other threads finished, Now this thread can run


int length = 3;
int args[3];

void worker() {
	int id = self_id();

	for (int i = 0; i < 5; i++) {
		printf("Thread: %d, Counter: %d\n", id, i);
		yield();
	}

	end_thread();
}

void t1_worker() {
	for (int i = 0; i < length; i++)
		join(args[i]);

	// waits for all other threads to finish
	printf("Other threads finished, Now this thread can run\n", self_id());

	end_thread();
}

int main() {
	init_lib();
	int main_thread = create_thread(NULL, 1);


	// t1 is created first with highest priority, but it will run at the end as it waits on all the other threads.
	int t1 = create_thread(t1_worker, 30);
	int t2 = create_thread(worker, 5);
	int t3 = create_thread(worker, 5);
	int t4 = create_thread(worker, 5);

	args[0] = t2;
	args[1] = t3;
	args[2] = t4;

	timer_start();
	join(t1);
}