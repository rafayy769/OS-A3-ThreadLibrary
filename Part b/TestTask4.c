#include "thread.h"


// Expected output:

// Thread: 2, Counter: 0
// Thread: 2, Counter: 1
// Thread: 2, Counter: 2
// Thread: 3, Counter: 0
// Thread: 2, Counter: 3
// Thread: 3, Counter: 1
// Thread: 2, Counter: 4
// Thread: 3, Counter: 2
// Thread: 3, Counter: 3
// Thread: 3, Counter: 4
// Thread: 4, Counter: 0
// Thread: 4, Counter: 1
// Thread: 4, Counter: 2
// Thread: 4, Counter: 3
// Thread: 4, Counter: 4
// Thread: 5, Counter: 0
// Thread: 5, Counter: 1
// Thread: 5, Counter: 2
// Thread: 5, Counter: 3
// Thread: 5, Counter: 4

void worker() {
	int id = self_id();

	for (int i = 0; i < 5; i++) {
		printf("Thread: %d, Counter: %d\n", id, i);
		yield();
	}

	end_thread();
}

int main() {
	init_lib();
	int main_thread = create_thread(NULL, 1);

	create_thread(worker, 30);
	create_thread(worker, 27);
	create_thread(worker, 20);
	create_thread(worker, 15);


	timer_start();
	while(1){
	}
}