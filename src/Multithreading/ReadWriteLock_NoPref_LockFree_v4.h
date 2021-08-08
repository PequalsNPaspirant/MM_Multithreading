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

	//TODO: This is Reader preferring lock. Fix to make it NoPref.

	namespace readWriteLock_NoPref_LockFree_v4 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				int oldNumReaders = numReaders_.fetch_add(1);
				if (oldNumReaders == 0)
				{
					while (readersOrWritterActive_.exchange(true))
					{
						std::this_thread::yield();
					}
					readersReadyToGo_.store(true, std::memory_order_release);
				}

				while (readersReadyToGo_.load(std::memory_order_acquire) == false)
					std::this_thread::yield();
			}

			void unlock_shared()
			{
				int oldNumReaders = numReaders_.fetch_sub(1);
				if (oldNumReaders == 1)
				{
					//reset the flags in reverse order they are set in lock_shared()
					readersReadyToGo_.store(false, std::memory_order_release);
					readersOrWritterActive_.store(false, std::memory_order_release);
				}
			}

			void lock()
			{
				while (numReaders_.load(std::memory_order_acquire) > 0
					|| readersOrWritterActive_.exchange(true))
				{
					std::this_thread::yield();
				}

				//int oldNumWriters = numWriters_.fetch_add(1);

				//while (numReaders_.load(std::memory_order_acquire) > 0
				//	|| numWriters_.load(std::memory_order_acquire) > 1)
				//	//|| writterActive_.load(std::memory_order_acquire) == true)
				//	std::this_thread::yield();

				//writterActive_.store(true, std::memory_order_release);
			}

			void unlock()
			{
				//--numWriters_; //equivalent to numWriters_.fetch_sub(1)
				readersOrWritterActive_.store(false, std::memory_order_release);
			}

		private:
			std::atomic<int> numReaders_{ 0 };
			//std::atomic<int> numWriters_{ 0 };
			std::atomic<bool> readersOrWritterActive_{ false };
			std::atomic<bool> readersReadyToGo_{ false };
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

