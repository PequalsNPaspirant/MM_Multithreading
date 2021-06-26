//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <string>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

/*

Implement condition_variable using semaphore:
https://www.microsoft.com/en-us/research/wp-content/uploads/2004/12/ImplementingCVs.pdf

*/

namespace mm {

	namespace ConditionVariableUsingSemaphore {

		//Basic OS Premitives - generally provided by Operating System

		//We need a OS defined semaphore. 
		//But instead of dealing with OS APIs, the below semaphore using std::condition_variable and mutex will be sufficient to test our implementation of condition variable

		class SemaphoreUsingConditionVariable
		{
		public:
			SemaphoreUsingConditionVariable()
			{}

			void release() {
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				++count_;
				condition_.notify_one();
			}

			void acquire() {
				std::unique_lock<decltype(mutex_)> lock(mutex_);
				while (count_ == 0) // Handle spurious wake-ups.
					condition_.wait(lock);
				--count_;
			}

			bool try_acquire() {
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if (count_ == 0)
					return false;

				--count_;
				return true;
			}

		private:
			std::mutex mutex_;
			std::condition_variable condition_;
			unsigned long count_ = 0; // Initialized as locked.
		};




		//Our implementation based on basic OS premitives

		class ConditionVariableUsingSemaphore
		{
		public:
			ConditionVariableUsingSemaphore()
			{
				m_.lock();
			}

			void wait(std::unique_lock<std::mutex>& lock)
			{
				lock.unlock();
				//block this thread
				m_.lock();
			}

			void notify_one()
			{
				//release blocked thread
				m_.unlock();
			}

			void notify_all()
			{
				//release all blocked threads
			}

		private:
			std::mutex m_;
			
		};

		class ThreadSafeQueue
		{
		public:
			void push(int n)
			{
				std::unique_lock<std::mutex> lock{ m_ };
				q_.push(n);
				lock.unlock();

				cv_.notify_one();
			}

			int pop()
			{
				std::unique_lock<std::mutex> lock{ m_ };

				while (q_.empty())
					cv_.wait(lock);

				int retVal = q_.front();
				q_.pop();
				return retVal;
			}

		private:
			std::queue<int> q_;
			std::mutex m_;
			ConditionVariableUsingSemaphore cv_;
		};

		void usage()
		{
			ThreadSafeQueue myQueue;
			std::atomic<bool> stop = false;
			auto threadFunPush = [&]() {
				while(!stop.load(std::memory_order_acquire))
					myQueue.push(10);
			};
			
			auto threadFunPop = [&]() {
				while (!stop.load(std::memory_order_acquire))
					int n = myQueue.pop();
			};
			
			int numThreads = 100;
			std::vector<std::thread> threadPool;
			threadPool.reserve(numThreads);
			for (int i = 0; i < numThreads; ++i)
			{
				threadPool.push_back(std::thread{ threadFunPush });
				threadPool.push_back(std::thread{ threadFunPop });
			}

			for (int i = 0; i < 2 * numThreads; ++i)
			{
				if (threadPool[i].joinable())
					threadPool[i].join();
			}

			std::this_thread::sleep_for(std::chrono::seconds(10));
			stop.store(false, std::memory_order_release);
		}
	}

}

