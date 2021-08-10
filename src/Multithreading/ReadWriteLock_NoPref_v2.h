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

//Reference: https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem

namespace mm {

	namespace readWriteLock_NoPref_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				//serviceQueue_.P();
				std::unique_lock<std::mutex> lock(serviceQueueMu_);
				while (serviceQueueCount_ == 0) // Handle spurious wake-ups.
					serviceQueueCv_.wait(lock);
				--serviceQueueCount_;
				lock.unlock();

				std::unique_lock<std::mutex> lock2{ mu_ };

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

				//serviceQueue_.V();
				std::unique_lock<std::mutex> lock4(serviceQueueMu_);
				++serviceQueueCount_;
				lock4.unlock();

				serviceQueueCv_.notify_one();

				lock2.unlock();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

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
				//serviceQueue_.P();
				std::unique_lock<std::mutex> lock(serviceQueueMu_);
				while (serviceQueueCount_ == 0) // Handle spurious wake-ups.
					serviceQueueCv_.wait(lock);
				--serviceQueueCount_;
				lock.unlock();

				//resource_.P();
				std::unique_lock<std::mutex> lock2(resourceMu_);
				while (resourceCount_ == 0) // Handle spurious wake-ups.
					resourceCv_.wait(lock2);
				--resourceCount_;
				lock2.unlock();
				
				//serviceQueue_.V();
				std::unique_lock<std::mutex> lock3(serviceQueueMu_);
				++serviceQueueCount_;
				lock3.unlock();

				serviceQueueCv_.notify_one();
			}

			void unlock()
			{
				//resource_.V();
				std::unique_lock<std::mutex> lock(resourceMu_);
				++resourceCount_;
				lock.unlock();

				resourceCv_.notify_one();
			}

		private:
			std::mutex mu_;
			//SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable resource_{ 1 };
			std::mutex resourceMu_;
			std::condition_variable resourceCv_;
			unsigned long resourceCount_ = { 1 };

			//SemaphoreUsingConditionVariable::SemaphoreUsingConditionVariable serviceQueue_{ 1 };
			std::mutex serviceQueueMu_;
			std::condition_variable serviceQueueCv_;
			unsigned long serviceQueueCount_ = { 1 };

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

