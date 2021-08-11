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

	namespace readWriteLock_WritePref_LockFree_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				//++numReadersWaiting_;
				int numWritersWaiting = 0;
				while ((numWritersWaiting = numWritersWaiting_.load(std::memory_order_acquire)) > 0 ||
					numActiveReadersWriters_.fetch_add(1, std::memory_order_seq_cst) > maxConcurrentReadersAllowed)
				{
					if(numWritersWaiting <= 0)
						--numActiveReadersWriters_;
					std::this_thread::yield();
				}
				//--numReadersWaiting_;
			}

			void unlock_shared()
			{
				--numActiveReadersWriters_; //equivalent to numReaders_.fetch_sub(1, std::memory_order_seq_cst)
			}

			void lock()
			{
				++numWritersWaiting_;
				//int numReadersWaiting = numReadersWaiting_.load(std::memory_order_acquire);
				while (//(numReadersWaiting = numReadersWaiting_.load(std::memory_order_acquire)) > 0 ||
					numActiveReadersWriters_.fetch_add(writerMask, std::memory_order_seq_cst) > 0)
				{
					//if(numReadersWaiting <= 0)
						numActiveReadersWriters_.fetch_sub(writerMask, std::memory_order_seq_cst);
					std::this_thread::yield();
				}
				--numWritersWaiting_;
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
			std::atomic<int> numWritersWaiting_{ 0 };
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

