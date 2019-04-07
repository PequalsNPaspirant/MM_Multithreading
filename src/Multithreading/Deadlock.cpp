/*
Deadlock and techniques to avoid it.

http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0156r2.html
https://www.justsoftwaresolutions.co.uk/threading/acquiring-multiple-locks-without-deadlock.html
https://stackoverflow.com/questions/17113619/whats-the-best-way-to-lock-multiple-stdmutexes/17113678
https://stackoverflow.com/questions/43019598/stdlock-guard-or-stdscoped-lock
https://stackoverflow.com/questions/17113619/whats-the-best-way-to-lock-multiple-stdmutexes/17113678

*/

#include <iostream>
#include <thread>
#include <functional> //std::function
using namespace std;

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	

	MM_DECLARE_FLAG(Multithreading_Deadlock_1);
	MM_UNIT_TEST(Multithreading_Deadlock_1_test, Multithreading_Deadlock_1)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		//Multithreading_basics_test_1();
		//Multithreading_basics_test_2();
		//Multithreading_basics_test_3();
		//Multithreading_basics_test_4();
		//Multithreading_basics_test_5();

	}

}

