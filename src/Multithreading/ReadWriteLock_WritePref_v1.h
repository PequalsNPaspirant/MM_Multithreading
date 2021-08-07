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

	namespace readWriteLock_WritePref_v1 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				readTry_.P();

				std::unique_lock<std::mutex> lock{ muReader_ };

				++numReadersActive_;

				if (numReadersActive_ == 1)
					resource_.P();

				lock.unlock();
				readTry_.V();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ muReader_ };
				--numReadersActive_;

				if (numReadersActive_ == 0)
					resource_.V();

				lock.unlock();
			}

			void lock()
			{
				std::unique_lock<std::mutex> lock{ muWriter_ };

				++numWritersActive_;

				if (numWritersActive_ == 1)
					readTry_.P();

				lock.unlock();

				resource_.P();
			}

			void unlock()
			{
				resource_.V();

				std::unique_lock<std::mutex> lock{ muWriter_ };

				--numWritersActive_;

				if (numWritersActive_ == 0)
					readTry_.V();

				lock.unlock();
			}

		private:
			std::mutex muReader_;
			std::mutex muWriter_;

			SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable readTry_{ 1 };
			SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable resource_{ 1 };

			int numReadersActive_{ 0 };
			int numWritersActive_{ 0 };
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

