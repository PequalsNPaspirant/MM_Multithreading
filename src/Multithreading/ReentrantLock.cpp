/*
Deadlock and techniques to avoid it.

https://stackoverflow.com/questions/17113619/whats-the-best-way-to-lock-multiple-stdmutexes/17113678
https://stackoverflow.com/questions/43019598/stdlock-guard-or-stdscoped-lock


*/

#include <iostream>
#include <thread>
#include <functional> //std::function
#include <mutex>
using namespace std;

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	//From wiki: https://en.wikipedia.org/wiki/Reentrant_mutex
	//In computer science, the reentrant mutex (recursive mutex, recursive lock) is a particular type of mutual exclusion (mutex) device that may be locked multiple times by the same process/thread, without causing a deadlock.
	//How a normal mutex can cause deadlock in a very simple situation:
	class ReentrantLockIssue
	{
	public:
		void memberFun_1()
		{
			std::unique_lock<std::mutex> lk(mu_);
			//Do something
		}

		void memberFun_2()
		{
			std::unique_lock<std::mutex> lk(mu_); //OR std::lock_gaurd<std::mutex> lk(mu_)
			//Do something
			memberFun_1(); //calling the function which tries to take a lock on the mutex which is already locked by this thread.
		}

	private:
		std::mutex mu_;
	};

	void deadlock_1()
	{
		ReentrantLockIssue obj;
		obj.memberFun_2(); //This will cause the deadlock
	}

	/*
	Recursive mutex can solve this issue:
	Ref: http://www.cplusplus.com/reference/mutex/recursive_mutex/lock/
	If the recursive_mutex isn't currently locked by any thread, the calling thread locks it (from this point, and until its member unlock is called, the thread owns the mutex).
	If the recursive_mutex is currently locked by another thread, execution of the calling thread is blocked until completely unlocked by the other thread (other non-locked threads continue their execution).
	If the recursive_mutex is currently locked by the same thread calling this function, the thread acquires a new level of ownership over the recursive_mutex. Unlocking the recursive_mutex completely will require an additional call to member unlock.
	*/

	

	MM_DECLARE_FLAG(Multithreading_ReentrantLock_1);
	MM_UNIT_TEST(Multithreading_ReentrantLock_1_test, Multithreading_ReentrantLock_1)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		deadlock_1();
	}

}

