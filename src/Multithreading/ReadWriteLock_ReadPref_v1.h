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

#include "SemaphoreUsingConditionVariable.h"

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

//Reference: https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock

namespace mm {

	namespace readWriteLock_ReadPref_v1 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				std::unique_lock<std::mutex> lock{ muReader_ };

				++numReadersActive_;

				if (numReadersActive_ == 1)
					semWriter_.P();

				lock.unlock();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ muReader_ };
				--numReadersActive_;

				if (numReadersActive_ == 0)
					semWriter_.V();

				lock.unlock();
			}

			void lock()
			{
				semWriter_.P();
			}

			void unlock()
			{
				semWriter_.V();
			}

		private:
			std::mutex muReader_;
			constexpr static const int numParallelWriters_{ 1 };
			SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable semWriter_{ numParallelWriters_ };

			int numReadersActive_{ 0 };
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

