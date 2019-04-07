// Test.cpp : Defines the entry point for the console application.

#include <iostream>

#include "WindowsUtils/WindowsUtils.h"
#include "Timer/Timer.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

using namespace mm;

namespace mm {
	MM_DEFINE_FLAG(false, Multithreading_basics_1);
	MM_DEFINE_FLAG(true, Multithreading_replaceMutexByAtomic_1);
	MM_DEFINE_FLAG(false, Multithreading_Deadlock_1);
	MM_DEFINE_FLAG(false, Multithreading_ReentrantLock_1);
}

int main(int argc, char* argv[])
{
	MaximizeWindow();				

	MM_RUN_UNIT_TESTS

	cout << "\n\n\n" << Timer::getCurrentLocalTimeInNanoSeconds() << " CONGRATULATIONS!!! End of program reached successfully.\n\n\n";

	std::cin.get();
	return 0;
}

