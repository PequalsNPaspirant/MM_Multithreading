#pragma once

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>
#include <string>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

/*
More info:
Part 1 below explains the history and problems with semaphore

https://blog.feabhas.com/2009/09/mutex-vs-semaphores-%E2%80%93-part-1-semaphores/
https://blog.feabhas.com/2009/09/mutex-vs-semaphores-%E2%80%93-part-2-the-mutex/
https://blog.feabhas.com/2009/10/mutex-vs-semaphores-%E2%80%93-part-3-final-part-mutual-exclusion-problems/

https://stackoverflow.com/questions/62814/difference-between-binary-semaphore-and-mutex

*/

namespace mm {

	namespace SemaphoreUsingConditionVariable {

		class SemaphoreUsingConditionVariable
		{
		public:
			SemaphoreUsingConditionVariable(unsigned long count = 0)
				: count_{ count }
			{}

			void release() {
				std::unique_lock<decltype(mutex_)> lock(mutex_);
				++count_;
				lock.unlock();

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

		class MutexUsingSemaphore
		{
		public:
			void lock()
			{
				s_.acquire();
			}

			void unlock()
			{
				s_.release();
			}

		private:
			SemaphoreUsingConditionVariable s_{ 1 }; //initialize with 1
		};
		

		class FixedSizeThreadSafeQueue
		{
		public:
			FixedSizeThreadSafeQueue(int maxSize) :
				maxSize_{ maxSize },
				sEmptySlots_{ maxSize },
				sFullSlots_{ 0 }
			{}

			void push(int n)
			{
				//std::unique_lock<std::mutex> lock{ m_ };
				//q_.push(n);
				//lock.unlock();

				//s_.release();

				sEmptySlots_.acquire();

				m_.lock();
				q_.push(n);
				m_.unlock();

				sFullSlots_.release();
			}

			int pop()
			{
				//std::unique_lock<std::mutex> lock{ m_ };

				//while (q_.empty())
				//{
				//	lock.unlock();
				//	s_.acquire();

				//	lock.lock();
				//}

				//int retVal = q_.front();
				//q_.pop();
				//return retVal;

				sFullSlots_.acquire();

				m_.lock();
				int retVal = q_.front();
				q_.pop();
				m_.unlock();

				sEmptySlots_.release();

				return retVal;
			}

		private:
			std::queue<int> q_;
			int maxSize_;
			MutexUsingSemaphore m_;
			SemaphoreUsingConditionVariable sEmptySlots_;
			SemaphoreUsingConditionVariable sFullSlots_;
		};

		class UnlimitedSizeThreadSafeQueue
		{
		public:
			UnlimitedSizeThreadSafeQueue() :
				//maxSize_{ maxSize },
				//sEmptySlots_{ maxSize },
				sFullSlots_{ 0 }
			{}

			void push(int n)
			{
				//std::unique_lock<std::mutex> lock{ m_ };
				//q_.push(n);
				//lock.unlock();

				//s_.release();

				//sEmptySlots_.acquire();

				m_.lock();
				q_.push(n);
				m_.unlock();

				sFullSlots_.release();
			}

			int pop()
			{
				//std::unique_lock<std::mutex> lock{ m_ };

				//while (q_.empty())
				//{
				//	lock.unlock();
				//	s_.acquire();

				//	lock.lock();
				//}

				//int retVal = q_.front();
				//q_.pop();
				//return retVal;

				sFullSlots_.acquire();

				m_.lock();
				int retVal = q_.front();
				q_.pop();
				m_.unlock();

				//sEmptySlots_.release();

				return retVal;
			}

		private:
			std::queue<int> q_;
			MutexUsingSemaphore m_;
			SemaphoreUsingConditionVariable sFullSlots_;
		};

		template<typename T>
		void usage(T& myQueue)
		{
			std::atomic<bool> stopPush{ false };
			std::atomic<bool> stopPop{ false };
			auto threadFunProducer = [&]() {
				while (!stopPush.load(std::memory_order_acquire))
					myQueue.push(10);
			};

			auto threadFunConsumer = [&]() {
				while (!stopPop.load(std::memory_order_acquire))
					int n = myQueue.pop();
			};

			cout << "\n" << "creating threads...";
			int numThreads = 100;
			std::vector<std::thread> threadPool;
			threadPool.reserve(numThreads);
			cout << "\n";
			for (int i = 0; i < numThreads; ++i)
			{
				if((i + 1) % 5 == 0)
					cout << "\r" << "created " << 2 * (i + 1) << " threads";
				threadPool.push_back(std::thread{ threadFunProducer });
				threadPool.push_back(std::thread{ threadFunConsumer });
			}
			cout << "\n" << "created all threads!";

			cout << "\n" << "sleeping for 10 sec...";
			std::this_thread::sleep_for(std::chrono::seconds(10));

			cout << "\n" << "stopping pop threads...";
			stopPop.store(true, std::memory_order_release);

			//Do not stop push threads immediately, there may be race
			//and some pop threads go into wait state and may never be 
			//notified because all push threads are stopped
			cout << "\n" << "sleeping for 5 sec...";
			std::this_thread::sleep_for(std::chrono::seconds(5));

			cout << "\n" << "stopping push threads...";
			stopPush.store(true, std::memory_order_release);

			cout << "\n" << "joining all threads...";
			for (int i = 0; i < 2 * numThreads; ++i)
			{
				if (threadPool[i].joinable())
					threadPool[i].join();
			}

			cout << "\n" << "All done!";
		}



		class PriorityControllerUsingSemaphore
		{
		public:
			void waitForHighPriorityTask()
			{
				s_.acquire();
			}

			void highPeiorityTaskDone()
			{
				s_.release();
			}

		private:
			SemaphoreUsingConditionVariable s_{ 0 }; //initialize with 0
		};

		void usagePriorityControllerUsingSemaphore()
		{
			PriorityControllerUsingSemaphore p;

			auto highPriorityTask = [&]() {
				//Step 1: Do high priority task

				//Step 2: Mark high prioity task as done so that low priority task can be started
				p.highPeiorityTaskDone();
			};

			auto lowPriorityTask = [&]() {
				//Step 1: Wait for high prioity task to finish before starting low priority task
				p.waitForHighPriorityTask();

				//Step 2: Do low priority task

			};

		}

	}

}

