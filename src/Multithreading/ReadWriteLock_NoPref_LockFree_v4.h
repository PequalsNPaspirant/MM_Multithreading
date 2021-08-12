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
				//while (!isReader.exchange(true))
				//	std::this_thread::yield();

				int oldNumReaders = numReadersWaiting_.fetch_add(1);

				size_t uniqueId = 0;
				//if (oldNumReaders == 0)
					uniqueId = uniqueId_.fetch_add(1);
				//else
				//	uniqueId = uniqueId_.load(std::memory_order_acquire);

				//size_t 
				while (uniqueId > uniqueIdCurrent_.load(std::memory_order_acquire))
					std::this_thread::yield();

				//++numReadersWaiting_;
				//++uniqueIdCurrent_;

				//int oldNumReaders = numReadersWaiting_.fetch_add(1);
				//if (oldNumReaders == 0)
				//{
				//	while (readersOrWritterActive_.exchange(true))
				//		//&& !readersReadyToGo_.load(std::memory_order_acquire))
				//		//|| numWritersWaiting_.load(std::memory_order_acquire) > 0
				//	{
				//		std::this_thread::yield();
				//	}
				//
				//	readersReadyToGo_.store(true, std::memory_order_release);
				//}

				//while (readersReadyToGo_.load(std::memory_order_acquire) == false)
				//	std::this_thread::yield();
			}

			void unlock_shared()
			{
				//int oldNumReaders = numReadersWaiting_.fetch_sub(1);
				//if (oldNumReaders == 1)
				//{
				//	//reset the flags in reverse order they are set in lock_shared()
				//	readersReadyToGo_.store(false, std::memory_order_release);
				//	readersOrWritterActive_.store(false, std::memory_order_release);
				//}
				//--numReadersWaiting_;
				//int oldNumReaders = numReadersWaiting_.fetch_sub(1);
				//if(oldNumReaders == 1)
					++uniqueIdCurrent_;
			}

			void lock()
			{
				//while (isReader.exchange(false))
				//	std::this_thread::yield();

				size_t uniqueId = uniqueId_.fetch_add(1);
				while (uniqueId > uniqueIdCurrent_.load(std::memory_order_acquire))
					std::this_thread::yield();

				//Reset reader count
				//numReadersWaiting_ = 0;
				//uniqueId_.fetch_add(1);

				//++uniqueIdCurrent_;

				//numWritersWaiting_.fetch_add(1);

				//while (numReadersWaiting_.load(std::memory_order_acquire) > 0
				//	//|| readersOrWritterActive_.exchange(true)
				//	)
				//{
				//	std::this_thread::yield();
				//}

				//numWritersWaiting_.fetch_sub(1);

				//int oldNumWriters = numWriters_.fetch_add(1);

				//while (numReadersWaiting_.load(std::memory_order_acquire) > 0
				//	|| numWriters_.load(std::memory_order_acquire) > 1)
				//	//|| writterActive_.load(std::memory_order_acquire) == true)
				//	std::this_thread::yield();

				//writterActive_.store(true, std::memory_order_release);
			}

			void unlock()
			{
				//--numWriters_; //equivalent to numWriters_.fetch_sub(1)
				//readersOrWritterActive_.store(false, std::memory_order_release);
				++uniqueIdCurrent_;
			}

		private:
			std::atomic<size_t> uniqueId_{ 0 };
			std::atomic<size_t> uniqueIdLastReader_{ 0 };
			std::atomic<size_t> uniqueIdCurrent_{ 0 };
			std::atomic<int> numReadersWaiting_{ 0 };
			std::atomic<int> numWritersWaiting_{ 0 };
			std::atomic<bool> readersOrWritterActive_{ false };
			std::atomic<bool> readersReadyToGo_{ false };
			std::atomic<bool> isReader{ false };
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

