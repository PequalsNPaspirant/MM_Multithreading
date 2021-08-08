//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	namespace readWriteLock_NoPref_LockFree_v3 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				while (readersOrWritterActive_.exchange(true))
				{
					std::this_thread::yield();
				}
			}

			void unlock_shared()
			{
				readersOrWritterActive_.store(false, std::memory_order_release);
			}

			void lock()
			{
				while (readersOrWritterActive_.exchange(true))
				{
					std::this_thread::yield();
				}
			}

			void unlock()
			{
				readersOrWritterActive_.store(false, std::memory_order_release);
			}

		private:
			std::atomic<bool> readersOrWritterActive_{ false };
		};

		/*
		All Cases:
		0 -                                                                        start


		1 -                                   R1                                                                          W1


		2 -               R2                                  W2                       	              R2                                  W2

		3 -       R3               W3                 R3               W3                     R3               W3                 R3               W3
		4 -    R4    W4         R4    W4		   R4    W4         R4    W4			   R4    W4         R4    W4		   R4    W4         R4    W4
		*/

		class ReadWriteLock
		{
		public:
			void acquireReadLock()
			{
				m_.lock_shared();
			}

			void releaseReadLock()
			{
				m_.unlock_shared();
			}

			void acquireWriteLock()
			{
				m_.lock();
			}

			void releaseWriteLock()
			{
				m_.unlock();
			}

		private:
			SharedMutex m_;
		};

	}

}

