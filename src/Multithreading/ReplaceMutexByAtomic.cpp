#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <cassert>
using namespace std;
using namespace std::chrono_literals;

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	/*
	Yes, we can use atomic variable in place of mutex, to achieve perfect synchonization, but there are two disadvantages:
	1. When lock is already acquired by another thread, it needs to spinlock and it wastes tons of CPU time/cycles 
	   because the waiting thread is actively polling. Mutex is an OS mechanism which sends the thread to sleep so 
	   the CPU core can do other stuff. But this in turn causes the system scheduler to peform a context switch which takes time as well.
	   So you are basically trading cpu time for response time.
	2. Mutex also employs safety measures to make sure only the locking thread can unlock it.
	

	Regarding memory order:
	https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync
	https://stackoverflow.com/questions/12346487/what-do-each-memory-order-mean
	https://bartoszmilewski.com/2008/12/01/c-atomics-and-memory-ordering/
	http://www.cplusplus.com/reference/atomic/memory_order/
	https://en.cppreference.com/w/cpp/atomic/memory_order
	*/

	//global variable to modify
	unsigned long long globalVal = 0;
	int numThreads = 1000;
	int numIterations = 100;
	std::chrono::milliseconds sleepTime(2);

	std::mutex m;
	void syncronizeUsingMutex(int increment)
	{
		std::unique_lock<std::mutex> lock(m);
		globalVal += increment;
	}

	std::atomic<bool> my_mutex_replacement(false);
	void syncronizeUsingAtomicVariable_unsafe(int increment)
	{
		while (true)
		{
			if (!my_mutex_replacement)
			{
				// OOPS!  someone could change it here.
				//my_mutex_replacement = true;
				my_mutex_replacement.store(true);
				globalVal += increment;
				my_mutex_replacement.store(false);
				break;
			}
			else
			{
				//Spin
				//Also you may make this thread sleep for some time to save CPU cycles
			}
		}
	}

	/*
	std::atomic_flag is an atomic boolean type. Unlike all specializations of std::atomic, it is guaranteed to be lock-free. Unlike std::atomic<bool>,
	std::atomic_flag does not provide load or store operations.
	*/
	std::atomic_flag lock1 = ATOMIC_FLAG_INIT;
	void syncronizeUsingAtomicVariable_safe_v1(int increment)
	{
		while (lock1.test_and_set(std::memory_order_acquire))  // acquire lock
             ; // spin
		globalVal += increment;
		lock1.clear(std::memory_order_release);               // release lock
	}

	/*
	std::atomic_flag is an atomic boolean type. Unlike all specializations of std::atomic, it is guaranteed to be lock-free. Unlike std::atomic<bool>,
	std::atomic_flag does not provide load or store operations.
	*/
	std::atomic_flag lock2 = ATOMIC_FLAG_INIT;
	void syncronizeUsingAtomicVariable_safe_v2(int increment)
	{
		while(std::atomic_flag_test_and_set_explicit(&lock2, std::memory_order_acquire))
             ; // spin until the lock is acquired
		globalVal += increment;
		std::atomic_flag_clear_explicit(&lock2, std::memory_order_release);
	}

	std::atomic<bool> lock3(false); // holds true when locked and false when unlocked
	void syncronizeUsingAtomicVariable_safe_v3(int increment)
	{
		while(std::atomic_exchange_explicit(&lock3, true, std::memory_order_acquire))
             ; // spin until acquired
		globalVal += increment;
		std::atomic_store_explicit(&lock3, false, std::memory_order_release);
	}

	std::atomic<bool> lock4(false);
	void syncronizeUsingAtomicVariable_safe_v4(int increment)
	{
		//bool expected = false;
		//bool desired = true;
		//while (!lock4.compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_relaxed))
		//{
		//	expected = false;
		//	MyAssert::myRunTimeAssert(expected == true);
		//}

		//globalVal += increment;

		while (true)
		{
			bool expected = false;
			bool desired = true;
			if (lock4.compare_exchange_weak(expected, desired, std::memory_order_release, std::memory_order_relaxed))
			{
				globalVal += increment;
				//cout << "\nthread: " << increment << " globalVal: " << globalVal;
				lock4.store(false);
				break;
			}
			else
			{
				assert(expected == true);
				expected = false;
			}
		}
	}

	std::atomic<bool> lock5(false);
	void syncronizeUsingAtomicVariable_safe_v5(int increment)
	{
		while (true)
		{
			if(!lock5.exchange(true)) //swaps atomic and passed-in non-atomic variables atomically
			{
				//Nobody acquired lock yet
				globalVal += increment;
				//cout << "\nthread: " << increment << " globalVal: " << globalVal;
				lock5.store(false);
				break;
			}
			else
			{
				//cout << "\nspinning thread: " << increment;
				//Somebody already acquired lock, so spin
				//Also you may make this thread sleep for some time to save CPU cycles
			}
		}
	}

	std::atomic<bool> lock6(false);
	void syncronizeUsingAtomicVariable_safe_v6(int increment)
	{
		//can not simply use the store as another thread might have just set it to true from false.
		//We have to set it to true only if it is false
		//lock6.store(true, std::memory_order_acquire); 
		//Use exchange instead
		//T exchange( T desired_new_value, std::memory_order order = std::memory_order_seq_cst )
		//returns the old value
		while (lock6.exchange(true, std::memory_order_acquire)) //swaps atomic and passed-in non-atomic variables atomically
			; //spin

		//Nobody acquired lock yet
		globalVal += increment;
		//cout << "\nthread: " << increment << " globalVal: " << globalVal;

		lock6.store(false, std::memory_order_release);
	}


	void threadFunction(int increment, int version)
	{
		for (int i = 0; i < numIterations; ++i)
		{
			switch (version)
			{
			case 1:
				syncronizeUsingMutex(increment);
				break;
			case 2:
				syncronizeUsingAtomicVariable_unsafe(increment);
				break;
			case 3:
				syncronizeUsingAtomicVariable_safe_v1(increment);
				break;
			case 4:
				syncronizeUsingAtomicVariable_safe_v2(increment);
				break;
			case 5:
				syncronizeUsingAtomicVariable_safe_v3(increment);
				break;
			case 6:
				syncronizeUsingAtomicVariable_safe_v4(increment);
				break;
			case 7:
				syncronizeUsingAtomicVariable_safe_v5(increment);
				break;
			case 8:
				syncronizeUsingAtomicVariable_safe_v6(increment);
				break;
			}
				
			//std::this_thread::sleep_for(2ms);
			std::this_thread::sleep_for(sleepTime);
		}
	}

	void test(int version)
	{
		globalVal = 0;
		//Create 100 threads and try to modify the variable
		vector<std::thread> threadVector(numThreads);
		for (int i = 0; i < threadVector.size(); ++i)
			threadVector[i] = std::thread(threadFunction, i + 1, version);

		for (int i = 0; i < threadVector.size(); ++i)
			threadVector[i].join();

		cout << "\nFinal Value: " << globalVal;
	}

	MM_DECLARE_FLAG(Multithreading_replaceMutexByAtomic_1);
	MM_UNIT_TEST(Multithreading_replaceMutexByAtomic_1_test, Multithreading_replaceMutexByAtomic_1)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		//use mutex
		test(1);
		//use atomic variable (unsafe)
		test(2);
		//use atomic variable (safe)
		test(3);
		test(4);
		test(5);
		test(6);
		test(7);
		test(8);
	}

/*
Output:

For 100 threads:
Final Value: 505000
Final Value: 504321
Final Value: 505000
Final Value: 505000
Final Value: 505000
Final Value: 505000
Final Value: 505000

For 1000 threads:
Final Value: 50050000
Final Value: 50014715
Final Value: 50050000
Final Value: 50050000
Final Value: 50050000
Final Value: 50050000
Final Value: 50050000

*/

}
