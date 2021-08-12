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

	namespace readWriteLock_ReadPref_LockFree_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				++numReadersWaiting_;
				int expected = numActiveReadersWriters_.load(std::memory_order_acquire);
				while (expected > maxConcurrentReadersAllowed ||
					!numActiveReadersWriters_.compare_exchange_weak(expected, expected + 1, std::memory_order_seq_cst))
				{
					expected = (expected > maxConcurrentReadersAllowed ? numActiveReadersWriters_.load(std::memory_order_acquire) : expected);
					std::this_thread::yield();
				}
				--numReadersWaiting_;
			}

			void unlock_shared()
			{
				--numActiveReadersWriters_; //equivalent to numReaders_.fetch_sub(1, std::memory_order_seq_cst)
			}

			void lock()
			{
				int expected = numActiveReadersWriters_.load(std::memory_order_acquire);
				while (expected > 0 ||
					numReadersWaiting_.load(std::memory_order_acquire) > 0 ||
					!numActiveReadersWriters_.compare_exchange_weak(expected, expected + writerMask, std::memory_order_seq_cst))
				{
					expected = (expected > 0 ? numActiveReadersWriters_.load(std::memory_order_acquire) : expected);
					std::this_thread::yield();
				}
			}

			void unlock()
			{
				numActiveReadersWriters_.fetch_sub(writerMask, std::memory_order_seq_cst);
			}

		private:
			/*

			|<--   writers  -->|  |<--  readers   -->|
			1111 1111  1111 1111  1111 1111  1111 1111
			
			*/

			//Info: This allows maximum concurrent readers equal to 'maxConcurrentReadersAllowed' to read the data
			std::atomic<int> numReadersWaiting_{ 0 };
			std::atomic<int> numActiveReadersWriters_{ 0 };
			static constexpr const int writerMask{ 1 << 16 };
			static constexpr const int maxConcurrentReadersAllowed{ writerMask - 1 };
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

