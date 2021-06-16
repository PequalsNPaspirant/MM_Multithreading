//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include <thread>
#include <random>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"
#include "ReadWriteLock_std_v1.h"

namespace mm {

	namespace readWriteLockTesting {

		void testReadWriteLock()
		{

		}

	}


	MM_DECLARE_FLAG(ReadWriteLock);

	MM_UNIT_TEST(ReadWriteLock_Test, ReadWriteLock)
	{
		readWriteLockTesting::testReadWriteLock();
	}
}

