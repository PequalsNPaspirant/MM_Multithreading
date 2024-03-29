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
				FlagType expected = numActiveReadersWriters_.load(std::memory_order_acquire);
				while (expected > maxConcurrentReadersAllowed ||
					!numActiveReadersWriters_.compare_exchange_weak(expected, expected + 1, std::memory_order_seq_cst))
				{
					expected = (expected > maxConcurrentReadersAllowed ? numActiveReadersWriters_.load(std::memory_order_acquire) : expected);
					std::this_thread::yield();
				}
			}

			void unlock_shared()
			{
				--numActiveReadersWriters_; //equivalent to numReaderWriters_.fetch_sub(1, std::memory_order_seq_cst)
			}

			void lock()
			{
				FlagType expected = numActiveReadersWriters_.load(std::memory_order_acquire);
				while (expected > 0 ||
					!numActiveReadersWriters_.compare_exchange_weak(expected, expected | writerMask, std::memory_order_seq_cst))
				{
					expected = (expected > 0 ? numActiveReadersWriters_.load(std::memory_order_acquire) : expected);
					std::this_thread::yield();
				}
			}

			void unlock()
			{
				numActiveReadersWriters_.fetch_and(writerMaskUnset, std::memory_order_seq_cst);
				//equivalent to numReaderWriters_ &= writerMaskUnset;
				//equivalent to numReaderWriters_ &= (~writerMask);
				//equivalent to numReaderWriters_.fetch_sub(writerMask, std::memory_order_seq_cst);
			}

		private:
			using FlagType = unsigned int;
			std::atomic<FlagType> numActiveReadersWriters_{ 0 };
			
			/*

			 |<--    readers update this part     -->|
			1111 1111  1111 1111  1111 1111  1111 1111

			^
			|
			Writers update just MSB

			*/

			//Info: This allows maximum concurrent readers equal to (writerMask - 1) to read the data
			//static constexpr const FlagType writerMask{ 1 << 31 };
			//OR
			//static constexpr const FlagType writerMask{ 1 << ((sizeof(FlagType) * 8) - 1) };
			//OR
			static constexpr const FlagType writerMask{ ~(std::numeric_limits<FlagType>::max() >> 1) };
			static constexpr const int maxConcurrentReadersAllowed{ writerMask - 1 };

			static constexpr const FlagType writerMaskUnset{ ~writerMask };
			//Note: maxConcurrentReadersAllowed and writerMaskUnset are same because writerMask is power of 2
			
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

