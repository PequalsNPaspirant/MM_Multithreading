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

	namespace readWriteLock_WritePref_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				//readTry_.P();
				std::unique_lock<std::mutex> lock(readTryMu_);
				while (readTryCount_ == 0) // Handle spurious wake-ups.
					readTryCv_.wait(lock);
				--readTryCount_;
				lock.unlock();

				std::unique_lock<std::mutex> lock2{ muReader_ };

				++numReadersActive_;

				if (numReadersActive_ == 1)
				{
					//resource_.P();
					std::unique_lock<std::mutex> lock3(resourceMu_);
					while (resourceCount_ == 0) // Handle spurious wake-ups.
						resourceCv_.wait(lock3);
					--resourceCount_;
					lock3.unlock();
				}

				lock2.unlock();

				//readTry_.V();
				std::unique_lock<std::mutex> lock4(readTryMu_);
				++readTryCount_;
				lock4.unlock();

				readTryCv_.notify_one();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ muReader_ };
				--numReadersActive_;

				if (numReadersActive_ == 0)
				{
					//resource_.V();
					std::unique_lock<std::mutex> lock2(resourceMu_);
					++resourceCount_;
					lock2.unlock();

					resourceCv_.notify_one();
				}

				lock.unlock();
			}

			void lock()
			{
				std::unique_lock<std::mutex> lock{ muWriter_ };

				++numWritersActive_;

				if (numWritersActive_ == 1)
				{
					//readTry_.P();
					std::unique_lock<std::mutex> lock2(readTryMu_);
					while (readTryCount_ == 0) // Handle spurious wake-ups.
						readTryCv_.wait(lock2);
					--readTryCount_;
					lock2.unlock();
				}

				lock.unlock();

				//resource_.P();
				std::unique_lock<std::mutex> lock3(resourceMu_);
				while (resourceCount_ == 0) // Handle spurious wake-ups.
					resourceCv_.wait(lock3);
				--resourceCount_;
				lock3.unlock();
			}

			void unlock()
			{
				//resource_.V();
				std::unique_lock<std::mutex> lock(resourceMu_);
				++resourceCount_;
				lock.unlock();

				resourceCv_.notify_one();

				std::unique_lock<std::mutex> lock2{ muWriter_ };

				--numWritersActive_;

				if (numWritersActive_ == 0)
				{
					//readTry_.V();
					std::unique_lock<std::mutex> lock3(readTryMu_);
					++readTryCount_;
					lock3.unlock();

					readTryCv_.notify_one();
				}

				lock2.unlock();
			}

		private:
			std::mutex muReader_;
			std::mutex muWriter_;

			//SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable readTry_{ 1 };
			std::mutex readTryMu_;
			std::condition_variable readTryCv_;
			unsigned long readTryCount_ = { 1 };

			//SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable resource_{ 1 };
			std::mutex resourceMu_;
			std::condition_variable resourceCv_;
			unsigned long resourceCount_ = { 1 };

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

