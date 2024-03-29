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

//#include "SemaphoreUsingConditionVariable.h"

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

//Reference: https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock

namespace mm {

	namespace readWriteLock_ReadPref_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				std::unique_lock<std::mutex> lock{ muReader_ };

				++numReadersActive_;

				if (numReadersActive_ == 1)
				{
					//semWriter_.P();
					std::unique_lock<std::mutex> lock2(writerMu_);
					while (writerCount_ == 0) // Handle spurious wake-ups.
						writerCv_.wait(lock2);
					--writerCount_;
					lock2.unlock();
				}

				lock.unlock();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ muReader_ };
				--numReadersActive_;

				if (numReadersActive_ == 0)
				{
					//semWriter_.V();
					std::unique_lock<std::mutex> lock2(writerMu_);
					++writerCount_;
					lock2.unlock();

					writerCv_.notify_one();
				}

				lock.unlock();
			}

			void lock()
			{
				//semWriter_.P();
				std::unique_lock<std::mutex> lock(writerMu_);
				while (writerCount_ == 0) // Handle spurious wake-ups.
					writerCv_.wait(lock);
				--writerCount_;
				lock.unlock();
			}

			void unlock()
			{
				//semWriter_.V();
				std::unique_lock<std::mutex> lock(writerMu_);
				++writerCount_;
				lock.unlock();

				writerCv_.notify_one();
			}

		private:
			std::mutex muReader_;
			constexpr static const int numParallelWriters_{ 1 };
			//SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable semWriter_{ numParallelWriters_ };
			std::mutex writerMu_;
			std::condition_variable writerCv_;
			unsigned long writerCount_ = { numParallelWriters_ };

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

