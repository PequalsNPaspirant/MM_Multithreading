/*
Deadlock and techniques to avoid it.

https://stackoverflow.com/questions/17113619/whats-the-best-way-to-lock-multiple-stdmutexes/17113678
https://stackoverflow.com/questions/43019598/stdlock-guard-or-stdscoped-lock


*/

#include <iostream>
#include <thread>
#include <functional> //std::function
using namespace std;

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	

	MM_DECLARE_FLAG(Multithreading_ReentrantLock_1);
	MM_UNIT_TEST(Multithreading_ReentrantLock_1_test, Multithreading_ReentrantLock_1)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		//Multithreading_basics_test_1();
		//Multithreading_basics_test_2();
		//Multithreading_basics_test_3();
		//Multithreading_basics_test_4();
		//Multithreading_basics_test_5();

	}

}

