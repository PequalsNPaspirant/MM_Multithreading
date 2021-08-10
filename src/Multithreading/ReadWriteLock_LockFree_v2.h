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

//THIS IS NOT REALLY A READ-WRITE LOCK. It's added just to demonstrate the lock free synchronization.

namespace mm {

	namespace readWriteLock_LockFree_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				bool expected = false;
				bool newVal = true;
				while (!readersOrWritterActive_.compare_exchange_weak(expected, newVal, std::memory_order_seq_cst))
				{
					expected = false;
					std::this_thread::yield();
				}
			}

			void unlock_shared()
			{
				readersOrWritterActive_.store(false, std::memory_order_release);
			}

			void lock()
			{
				bool expected = false;
				bool newVal = true;
				while (!readersOrWritterActive_.compare_exchange_weak(expected, newVal, std::memory_order_seq_cst))
				{
					expected = false;
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

