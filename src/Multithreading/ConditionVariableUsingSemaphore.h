#pragma once

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

Documents on drive:
...\@_Programming\@_Multi-threading\ImplementingConditionVariables.pdf
...\@_Programming\@_Multi-threading\Semaphores.pdf

*/

namespace mm {

	namespace ConditionVariableUsingSemaphore {

		//Basic OS Premitives - generally provided by Operating System

		//We need a OS defined semaphore. 
		//But instead of dealing with OS APIs, the below semaphore using std::condition_variable and mutex will be sufficient to test our implementation of condition variable

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




		//Our implementation of ConditionVariable based on basic OS premitives

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

		struct ThreadInfo
		{
			SemaphoreUsingConditionVariable s_{ 0 };
		};

		//This class stores mappings for all threads irrespective of whether they
		//are using same cv or not
		class GlobalThreadInfoMappng
		{
		public:
			static std::shared_ptr<ThreadInfo> getThreadInfo()
			{
				GlobalThreadInfoMappng& inst = GlobalThreadInfoMappng::getInstance();
				inst.m_.lock();
				std::thread::id threadId = std::this_thread::get_id();
				std::shared_ptr<ThreadInfo>& retVal = inst.data_[threadId];
				if (!retVal)
					retVal = std::make_shared<ThreadInfo>();
				inst.m_.unlock();

				return retVal;
			}

		private:
			GlobalThreadInfoMappng() = default;
			static GlobalThreadInfoMappng& getInstance()
			{
				static GlobalThreadInfoMappng inst;
				return inst;
			}

			std::unordered_map< std::thread::id, std::shared_ptr<ThreadInfo> > data_;
			MutexUsingSemaphore m_;
		};

		class ConditionVariableUsingSemaphore
		{
		public:
			ConditionVariableUsingSemaphore()
			{
			}

			void wait(MutexUsingSemaphore& pMutex)
			{
				std::shared_ptr<ThreadInfo> ti = GlobalThreadInfoMappng::getThreadInfo();
				
				m_.lock();
				q_.push(ti);
				m_.unlock();

				pMutex.unlock();
				//block this thread
				ti->s_.acquire();
				pMutex.lock();
			}

			void notify_one()
			{
				m_.lock();
				//release blocked thread
				if (!q_.empty())
				{
					std::shared_ptr<ThreadInfo> ti = q_.front();
					q_.pop();
					ti->s_.release();
				}
				m_.unlock();
			}

			void notify_all()
			{
				m_.lock();
				//release all blocked thread
				while (!q_.empty())
				{
					std::shared_ptr<ThreadInfo> ti = q_.front();
					q_.pop();
					ti->s_.release();
				}
				m_.unlock();
			}

		private:
			std::queue< std::shared_ptr<ThreadInfo> > q_;
			MutexUsingSemaphore m_;
		};

		class UnlimitedSizeThreadSafeQueue
		{
		public:
			void push(int n)
			{
				m_.lock();
				q_.push(n);
				m_.unlock();

				cv_.notify_one();
			}

			int pop()
			{
				m_.lock();

				while (q_.empty())
					cv_.wait(m_);

				int retVal = q_.front();
				q_.pop();

				m_.unlock();

				return retVal;
			}

		private:
			std::queue<int> q_;
			MutexUsingSemaphore m_;
			ConditionVariableUsingSemaphore cv_;
		};

		void usage()
		{
			UnlimitedSizeThreadSafeQueue myQueue;
			std::atomic<bool> stopPush{ false };
			std::atomic<bool> stopPop{ false };
			auto threadFunPush = [&]() {
				while(!stopPush.load(std::memory_order_acquire))
					myQueue.push(10);
			};
			
			auto threadFunPop = [&]() {
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
				if ((i + 1) % 5 == 0)
					cout << "\r" << "created " << 2 * (i + 1) << " threads";
				threadPool.push_back(std::thread{ threadFunPush });
				threadPool.push_back(std::thread{ threadFunPop });
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

			for (int i = 0; i < 2 * numThreads; ++i)
			{
				if (threadPool[i].joinable())
					threadPool[i].join();
			}

			cout << "\n" << "All done!";
		}
	}

}

