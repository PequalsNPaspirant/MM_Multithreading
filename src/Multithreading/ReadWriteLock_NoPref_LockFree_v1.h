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

	namespace readWriteLock_NoPref_LockFree_v1 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				++numReaders_; //equivalent to numReaders_.fetch_add(1)

				while (numWriters_.load(std::memory_order_acquire) > 0)
					std::this_thread::yield();
			}

			void unlock_shared()
			{
				--numReaders_; //equivalent to numReaders_.fetch_sub(1)
			}

			void lock()
			{
				++numWriters_; //equivalent to numWriters_.fetch_add(1)

				while (numReaders_.load(std::memory_order_acquire) > 0
					|| writterActive_.load(std::memory_order_acquire) == true)
					std::this_thread::yield();

				writterActive_.store(true, std::memory_order_release);
			}

			void unlock()
			{
				--numWriters_; //equivalent to numWriters_.fetch_sub(1)
				writterActive_.store(false, std::memory_order_release);
			}

		private:
			std::atomic<int> numReaders_{ 0 };
			std::atomic<int> numWriters_{ 0 };
			std::atomic<bool> writterActive_{ false };
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

