#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <cassert> //for assert()
#include <cmath>
using namespace std;

//#include "Multithreading\Multithreading_SingleProducerMultipleConsumers_v1.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

/*
This is Multi Producers Multi Consumers Fixed Size Queue.
This is very common and basic implemention using one mutex and two condition variables (one each for producers and consumers).
It uses std::vector of fixed length to store data.
Producers have to wait if the queue is full.
Consumers have to wait if the queue is empty.
*/

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeQueue_v1
	{
	public:
		MultiProducersMultiConsumersFixedSizeQueue_v1(size_t maxSize)
			: maxSize_(maxSize), vec_(maxSize), size_(0), head_(0), tail_(0)
		{
		}

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while(size_ == maxSize_)
			{
				cvProducers_.wait(mlock);
			}
			vec_[head_] = std::move(obj);
			head_ = ++head_ % maxSize_;
			++size_;
			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			mlock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cvConsumers_.notify_one();
		}

		//exception UNSAFE pop() version
		T pop()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while(size_ == 0)
			{
				cvConsumers_.wait(mlock);
			}
			//OR we can use below
			//cond_.wait(mlock, [this](){ return this->size_ != 0; });
			auto obj = vec_[tail_];
			tail_ = ++tail_ % maxSize_;
			--size_;
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;

			mlock.unlock();
			cvProducers_.notify_one();

			return obj; //this object can be returned by copy/move and it will be lost if the copy/move constructor throws exception.
		}

		//exception SAFE pop() version
		//void pop(T& outVal) { //same implementation as above except the return statement. }

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (size_ == 0)
			{
				cvConsumers_.wait_for(mlock, timeout);
			}
			//OR we can use below
			//cond_.wait_for(mlock, timeout, [this](){ return this->size_ != 0; });
			auto obj = vec_[tail_];
			tail_ = ++tail_ % maxSize_;
			--size_;
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;

			mlock.unlock();
			cvProducers_.notify_one();
			outVal = obj;
		}

		size_t size()
		{
			return size_;
		}

		bool empty()
		{
			//return vec_.empty(); //vector is never empty. The elements will be overwritten by push if the queue is already full.
			return size_ == 0;
		}

	private:
		size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		size_t size_;
		size_t head_; //stores the index where next element will be pushed
		size_t tail_; //stores the index of object which will be popped
		std::mutex mutex_;
		std::condition_variable cvProducers_;
		std::condition_variable cvConsumers_;
	};
	
}