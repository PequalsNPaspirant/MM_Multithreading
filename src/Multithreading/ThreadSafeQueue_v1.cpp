
/*
Reference:
http://natsys-lab.blogspot.com/2013/05/lock-free-multi-producer-multi-consumer.html
https://stackoverflow.com/questions/8918401/does-a-multiple-producer-single-consumer-lock-free-queue-exist-for-c
https://stackoverflow.com/questions/2151780/is-multiple-producer-single-consumer-possible-in-a-lockfree-setting
https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
https://github.com/facebook/folly/blob/master/folly/ProducerConsumerQueue.h
https://stackoverflow.com/questions/1164023/is-there-a-production-ready-lock-free-queue-or-hash-implementation-in-c
https://stackoverflow.com/questions/8918401/does-a-multiple-producer-single-consumer-lock-free-queue-exist-for-c
http://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++
	https://github.com/cameron314/readerwriterqueue
https://groups.google.com/forum/#!topic/comp.programming.threads/M_ecdRRlgvM

http://www.1024cores.net/home/lock-free-algorithms/introduction
	http://www.1024cores.net/home/lock-free-algorithms/queues
	http://www.1024cores.net/home/lock-free-algorithms/queues/queue-catalog
	http://www.1024cores.net/home/lock-free-algorithms/queues/unbounded-spsc-queue
	http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue

https://en.wikipedia.org/wiki/Synchronization_(computer_science)
https://en.wikipedia.org/wiki/Monitor_(synchronization)#Condition_variables
https://en.wikipedia.org/wiki/Mutual_exclusion

https://en.cppreference.com/w/cpp/thread
	https://en.cppreference.com/w/cpp/thread/condition_variable
	https://en.cppreference.com/w/cpp/thread/mutex

https://www.linkedin.com/pulse/lock-free-single-producer-consumer-queue-c11-sander-jobing/

http://www.acodersjourney.com/2017/08/top-20-cplusplus-multithreading-mistakes/
*/

/*
Why condition variable needs a mutex?

Checking the predicate and waiting are not performed atomically in std::condition_variable::wait (unlocking the lock and sleeping are performed atomically). If it is possible for another thread to change the value of the predicate while this thread holds the mutex, then it is possible for notifications to occur between the predicate check and going to sleep, and effectively be lost.
Reference: https://stackoverflow.com/questions/21439359/signal-on-condition-variable-without-holding-lock?rq=1
https://www.google.com/search?q=c%2B%2B+condition+variable+without+mutex&rlz=1C1CHZL_enUS723US723&oq=C%2B%2B+why+condition+variable&aqs=chrome.4.69i57j69i60j0l4.11031j0j7&sourceid=chrome&ie=UTF-8

Synchonization premitives:
https://people.eecs.berkeley.edu/~kubitron/courses/cs162-F07/hand-outs/synch.html
https://www.justsoftwaresolutions.co.uk/threading/locks-mutexes-semaphores.html

Acquiring multiple mutexes:
https://www.justsoftwaresolutions.co.uk/threading/acquiring-multiple-locks-without-deadlock.html

*/


/*

Without synchronization:

global RingBuffer queue; // A thread-unsafe ring-buffer of tasks.

// Method representing each producer thread's behavior:
public method producer(){
    while(true){
        task myTask=...; // Producer makes some new task to be added.
        while(queue.isFull()){} // Busy-wait until the queue is non-full.
        queue.enqueue(myTask); // Add the task to the queue.
    }
}

// Method representing each consumer thread's behavior:
public method consumer(){
    while(true){
        while (queue.isEmpty()){} // Busy-wait until the queue is non-empty.
        myTask=queue.dequeue(); // Take a task off of the queue.
        doStuff(myTask); // Go off and do something with the task.
    }
}



With spin waiting using mutex:

global RingBuffer queue; // A thread-unsafe ring-buffer of tasks.
global Lock queueLock; // A mutex for the ring-buffer of tasks.

// Method representing each producer thread's behavior:
public method producer(){
    while(true){
        task myTask=...; // Producer makes some new task to be added.

        queueLock.acquire(); // Acquire lock for initial busy-wait check.
        while(queue.isFull()){ // Busy-wait until the queue is non-full.
            queueLock.release();
            // Drop the lock temporarily to allow a chance for other threads
            // needing queueLock to run so that a consumer might take a task.
            queueLock.acquire(); // Re-acquire the lock for the next call to "queue.isFull()".
        }

        queue.enqueue(myTask); // Add the task to the queue.
        queueLock.release(); // Drop the queue lock until we need it again to add the next task.
    }
}

// Method representing each consumer thread's behavior:
public method consumer(){
    while(true){
        queueLock.acquire(); // Acquire lock for initial busy-wait check.
        while (queue.isEmpty()){ // Busy-wait until the queue is non-empty.
            queueLock.release();
            // Drop the lock temporarily to allow a chance for other threads
            // needing queueLock to run so that a producer might add a task.
            queueLock.acquire(); // Re-acquire the lock for the next call to "queue.isEmpty()".
        }
        myTask=queue.dequeue(); // Take a task off of the queue.
        queueLock.release(); // Drop the queue lock until we need it again to take off the next task.
        doStuff(myTask); // Go off and do something with the task.
    }
}


using two condition variables:

global volatile RingBuffer queue; // A thread-unsafe ring-buffer of tasks.
global Lock queueLock;  	// A mutex for the ring-buffer of tasks. (Not a spin-lock.)
global CV queueEmptyCV; 	// A condition variable for consumer threads waiting for the queue to 
				// become non-empty.
                        	// Its associated lock is "queueLock".
global CV queueFullCV; 		// A condition variable for producer threads waiting for the queue 
				// to become non-full. Its associated lock is also "queueLock".

// Method representing each producer thread's behavior:
public method producer(){
    while(true){
        task myTask=...; // Producer makes some new task to be added.

        queueLock.acquire(); // Acquire lock for initial predicate check.
        while(queue.isFull()){ // Check if the queue is non-full.
            // Make the threading system atomically release queueLock,
            // enqueue this thread onto queueFullCV, and sleep this thread.
            wait(queueLock, queueFullCV);
            // Then, "wait" automatically re-acquires "queueLock" for re-checking
            // the predicate condition.
        }
        
        // Critical section that requires the queue to be non-full.
        // N.B.: We are holding queueLock.
        queue.enqueue(myTask); // Add the task to the queue.

        // Now the queue is guaranteed to be non-empty, so signal a consumer thread
        // or all consumer threads that might be blocked waiting for the queue to be non-empty:
        signal(queueEmptyCV); -- OR -- notifyAll(queueEmptyCV);
        
        // End of critical sections related to the queue.
        queueLock.release(); // Drop the queue lock until we need it again to add the next task.
    }
}

// Method representing each consumer thread's behavior:
public method consumer(){
    while(true){

        queueLock.acquire(); // Acquire lock for initial predicate check.
        while (queue.isEmpty()){ // Check if the queue is non-empty.
            // Make the threading system atomically release queueLock,
            // enqueue this thread onto queueEmptyCV, and sleep this thread.
            wait(queueLock, queueEmptyCV);
            // Then, "wait" automatically re-acquires "queueLock" for re-checking
            // the predicate condition.
        }
        // Critical section that requires the queue to be non-empty.
        // N.B.: We are holding queueLock.
        myTask=queue.dequeue(); // Take a task off of the queue.
        // Now the queue is guaranteed to be non-full, so signal a producer thread
        // or all producer threads that might be blocked waiting for the queue to be non-full:
        signal(queueFullCV); -- OR -- notifyAll(queueFullCV);

        // End of critical sections related to the queue.
        queueLock.release(); // Drop the queue lock until we need it again to take off the next task.

        doStuff(myTask); // Go off and do something with the task.
    }
}


using only one condition variable (Here we can not use notifyOne(), we must use notifyAll()):

global volatile RingBuffer queue; // A thread-unsafe ring-buffer of tasks.
global Lock queueLock; // A mutex for the ring-buffer of tasks.  (Not a spin-lock.)
global CV queueFullOrEmptyCV; // A single condition variable for when the queue is not ready for any thread
                              // -- i.e., for producer threads waiting for the queue to become non-full 
                              // and consumer threads waiting for the queue to become non-empty.
                              // Its associated lock is "queueLock".
                              // Not safe to use regular "signal" because it is associated with
                              // multiple predicate conditions (assertions).

// Method representing each producer thread's behavior:
public method producer(){
    while(true){
        task myTask=...; // Producer makes some new task to be added.

        queueLock.acquire(); // Acquire lock for initial predicate check.
        while(queue.isFull()){ // Check if the queue is non-full.
            // Make the threading system atomically release queueLock,
            // enqueue this thread onto the CV, and sleep this thread.
            wait(queueLock, queueFullOrEmptyCV);
            // Then, "wait" automatically re-acquires "queueLock" for re-checking
            // the predicate condition.
        }
        
        // Critical section that requires the queue to be non-full.
        // N.B.: We are holding queueLock.
        queue.enqueue(myTask); // Add the task to the queue.

        // Now the queue is guaranteed to be non-empty, so signal all blocked threads
        // so that a consumer thread will take a task:
        notifyAll(queueFullOrEmptyCV); // Do not use "signal" (as it might wake up another producer instead).
        
        // End of critical sections related to the queue.
        queueLock.release(); // Drop the queue lock until we need it again to add the next task.
    }
}

// Method representing each consumer thread's behavior:
public method consumer(){
    while(true){

        queueLock.acquire(); // Acquire lock for initial predicate check.
        while (queue.isEmpty()){ // Check if the queue is non-empty.
            // Make the threading system atomically release queueLock,
            // enqueue this thread onto the CV, and sleep this thread.
            wait(queueLock, queueFullOrEmptyCV);
            // Then, "wait" automatically re-acquires "queueLock" for re-checking
            // the predicate condition.
        }
        // Critical section that requires the queue to be non-full.
        // N.B.: We are holding queueLock.
        myTask=queue.dequeue(); // Take a task off of the queue.

        // Now the queue is guaranteed to be non-full, so signal all blocked threads
        // so that a producer thread will take a task:
        notifyAll(queueFullOrEmptyCV); // Do not use "signal" (as it might wake up another consumer instead).

        // End of critical sections related to the queue.
        queueLock.release(); // Drop the queue lock until we need it again to take off the next task.

        doStuff(myTask); // Go off and do something with the task.
    }
}

*/
