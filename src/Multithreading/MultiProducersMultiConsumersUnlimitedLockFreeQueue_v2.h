#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <cassert> //for assert()
#include <cmath>
#include <atomic>
using namespace std;

/*
TODO:
This is Multi Producers Multi Consumers Unlimited Size Queue.
This is implemented using four atomic boolean flags, 2 for producers and 2 for consumers and it uses spin locks.
It uses xxx to store data.
Producers never need to wait because its unlimited queue and will never be full.
Consumers have to wait if the queue is empty.
*/

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersUnlimitedLockFreeQueue_v2
	{
	public:

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			queue_.push(std::move(obj));
			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
			mlock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cond_.notify_one(); //This will always notify one thread even though there are no waiting threads
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty())
			{
				//cond_.wait(mlock);
				if(cond_.wait_for(mlock, timeout) == std::cv_status::timeout)
					return false;
			}
			//OR we can use below
			//cond_.wait_for(mlock, timeout, [this](){ return !this->queue_.empty(); });
			outVal = queue_.front();
			queue_.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true;
		}

		size_t size()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			return queue_.size();
		}

		bool empty()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			return queue_.empty();
		}

	private:
		std::queue<T> queue_;
		std::atomic<size_t> head_; //stores the index where next element will be pushed
		std::atomic<size_t> tail_; //stores the index of object which will be popped
	};
}