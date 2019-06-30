/*
https://stackoverflow.com/questions/6319146/c11-introduced-a-standardized-memory-model-what-does-it-mean-and-how-is-it-g?rq=1
http://www.justsoftwaresolutions.co.uk/threading/multithreading-in-c++0x-part-6-double-checked-locking.html
http://bartoszmilewski.wordpress.com/2008/12/01/c-atomics-and-memory-ordering/
http://dl.acm.org/citation.cfm?id=359545.359563
http://www.morganclaypool.com/doi/abs/10.2200/S00346ED1V01Y201104CAC016
http://philosophyfaculty.ucsd.edu/faculty/ccallender/index.html

Herb Sutter has a three hour long talk about the C++11 memory model titled "atomic<> Weapons", 
available on the Channel9 site - part 1 and part 2. The talk is pretty technical, and covers the following topics:

Optimizations, Races, and the Memory Model
Ordering – What: Acquire and Release
Ordering – How: Mutexes, Atomics, and/or Fences
Other Restrictions on Compilers and Hardware
Code Gen & Performance: x86/x64, IA64, POWER, ARM
Relaxed Atomics

part 1: http://channel9.msdn.com/Shows/Going+Deep/Cpp-and-Beyond-2012-Herb-Sutter-atomic-Weapons-1-of-2
part 2: http://channel9.msdn.com/Shows/Going+Deep/Cpp-and-Beyond-2012-Herb-Sutter-atomic-Weapons-2-of-2

http://msdn.microsoft.com/en-us/library/12a04hfd(v=vs.80).aspx

https://www.youtube.com/watch?v=LL8wkskDlbs&list=PL5jc9xFGsL8E12so1wlMS0r0hTQoJL74M

Mltithreading tutorial:
https://www.youtube.com/watch?v=LL8wkskDlbs

*/

#include <iostream>
#include <thread>
#include <functional> //std::function
using namespace std;

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	//Construct threads using C++ global functions. It prints Output-1 below.
	void threadFunction()
	{
		for(int i = 0; i < 50; ++i)
			cout << "\nI am in another thread";
	}

	void Multithreading_basics_test_1()
	{
		std::thread t1(threadFunction);

		for(int i = 0; i < 50; ++i)
			cout << "\nI am in main thread";

		t1.join();
	}

	//Construct threads using callable objects/functors
	class threadFunctor
	{
	public:
		void operator()()
		{
			for(int i = 0; i < 50; ++i)
				cout << "\nI am in another thread";
		}
	};
	void Multithreading_basics_test_2()
	{
		//std::thread t1(threadFunctor()); //Most vexing parse
		std::thread t1((threadFunctor()));

		for(int i = 0; i < 50; ++i)
			cout << "\nI am in main thread";

		t1.join();
	}

	//Construct threads using closures
	void Multithreading_basics_test_3()
	{
		std::thread t1([]() -> void {
							for(int i = 0; i < 50; ++i)
								cout << "\nI am in thread t1";
						});

		std::thread t2([]() {
							for(int i = 0; i < 50; ++i)
								cout << "\nI am in thread t2";
						});

		for(int i = 0; i < 50; ++i)
			cout << "\nI am in main thread";

		t1.join();
		t2.join();
	}

	//Construct threads using function objects
	void Multithreading_basics_test_4()
	{
		std::function<void(void)> fun = threadFunction;
		std::thread t1(fun);

		for(int i = 0; i < 50; ++i)
			cout << "\nI am in main thread";

		t1.join();
	}

	//Construct threads using member function
	class ThreadFunction
	{
	public:
		void memFun()
		{
			for(int i = 0; i < 50; ++i)
				cout << "\nI am in another thread";
		}

		std::thread spawn() 
		{
			return std::thread(&ThreadFunction::memFun, this);
		}
	};
	void Multithreading_basics_test_5()
	{
		std::thread t1(&ThreadFunction::memFun, ThreadFunction());
		ThreadFunction tf;
		std::thread t2 = tf.spawn();

		for(int i = 0; i < 50; ++i)
			cout << "\nI am in main thread";

		t1.join();
		t2.join();
	}

	MM_DECLARE_FLAG(Multithreading_basics_1);
	MM_UNIT_TEST(Multithreading_basics_1_test, Multithreading_basics_1)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		//Multithreading_basics_test_1();
		//Multithreading_basics_test_2();
		//Multithreading_basics_test_3();
		//Multithreading_basics_test_4();
		//Multithreading_basics_test_5();

	}

}


/*
	Output-1

I am in main thread
I am in another thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in another thread
I am in main thread
I am in main thread
I am in another thread
I am in main thread
I am in main thread
I am in main thread
I am in another thread
I am in main thread
I am in main thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in main thread
I am in another thread
I am in another thread
I am in another thread
I am in main thread
I am in another thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in another thread
I am in another thread
I am in main thread
I am in main thread
I am in main thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in main thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread
I am in another thread

	*/