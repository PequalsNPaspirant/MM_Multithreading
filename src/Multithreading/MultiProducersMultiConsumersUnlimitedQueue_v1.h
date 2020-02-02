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
#include <forward_list>
using namespace std;

/*
This is Multi Producers Multi Consumers Unlimited Size Queue.
This is the most common and basic implementation using one mutex and one condition variable.
It uses std::queue<T, Container<T>> to store data where Container is std::vector, std::list and std::forward_list
Producers never need to wait because its unlimited queue and will never be full.
Consumers have to wait if the queue is empty.
*/

namespace mm {

	//template <typename T, typename Container>
	template<typename T, template <typename... Args> class Container = std::deque>
	class MultiProducersMultiConsumersUnlimitedQueue_v1
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

		//exception SAFE pop() version
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty()) //If the thread is active due to spurious wake-up or more number of threads are notified than the number of elements in queue, 
				//force it to check if queue is empty so that it can wait again if the queue is empty
			{
				//cond_.wait(mlock);
				if (cond_.wait_for(mlock, timeout) == std::cv_status::timeout)
					return false;
			}
			//OR
			//cond_.wait(mlock, [this](){ return !this->queue_.empty(); });
			//cond_.wait_for(mlock, timeout, [this](){ return !this->queue_.empty(); });

			outVal = queue_.front();
			queue_.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true; //this object can be returned by copy/move and it will be lost if the copy/move constructor throws exception.
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
		std::queue<T, Container<T>> queue_; //The queue internally uses the vector by default
		std::mutex mutex_;
		std::condition_variable cond_;
	};



	template<typename T>
	class MultiProducersMultiConsumersUnlimitedQueue_v1<T, std::forward_list>
	{
	public:
		MultiProducersMultiConsumersUnlimitedQueue_v1()
			: last_{ queue_.before_begin() }, size_{ 0 }
		{}

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			last_ = queue_.insert_after(last_, std::move(obj)); //Push element at the tail.
			++size_;
			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
			mlock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cond_.notify_one(); //This will always notify one thread even though there are no waiting threads
		}

		//exception SAFE pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty()) //If the thread is active due to spurious wake-up or more number of threads are notified than the number of elements in queue, 
								   //force it to check if queue is empty so that it can wait again if the queue is empty
			{
				//cond_.wait(mlock);
				if (cond_.wait_for(mlock, timeout) == std::cv_status::timeout)
					return false;
			}
			//OR
			//cond_.wait(mlock, [this](){ return !this->queue_.empty(); });
			//cond_.wait_for(mlock, timeout, [this](){ return !this->queue_.empty(); });

			outVal = queue_.front();
			queue_.erase_after(queue_.before_begin());
			if (queue_.empty())
				last_ = queue_.before_begin();
			--size_;
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true;
		}

		size_t size()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			return std::distance(queue_.begin(), queue_.end());
		}

		bool empty()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			return queue_.empty();
		}

	private:
		std::forward_list<T> queue_; //The queue internally uses the vector by default
		typename std::forward_list<T>::iterator last_;
		size_t size_;
		std::mutex mutex_;
		std::condition_variable cond_;
	};
}