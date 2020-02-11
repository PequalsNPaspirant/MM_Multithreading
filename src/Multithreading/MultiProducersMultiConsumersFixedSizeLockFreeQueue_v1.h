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
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1(size_t maxSize)
			: maxSize_(maxSize), 
			vec_(maxSize), 
			//size_a{ 0 },
			head_{ 0 },
			tail_{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t localHead;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localHead = head_.load();

			} while (localHead - tail_.load() == maxSize_ || !head_.compare_exchange_weak(localHead, localHead + 1));
			vec_[localHead % maxSize_] = std::move(obj);



			//while (head_.load() - tail_.load() == maxSize_)
			//{
			//}

			////size_t localHead = head_++;
			//size_t localHead = head_.fetch_add(1);
			//vec_[localHead % maxSize_] = std::move(obj);

			//if (head_ >= maxSize_)
			//	head_ = head_ % maxSize_;

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t localTail;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localTail = tail_.load();

			} while (head_.load() == localTail || !tail_.compare_exchange_weak(localTail, localTail + 1));
			outVal = std::move(vec_[localTail % maxSize_]);



			//while (head_.load() == tail_.load())
			//{
			//}

			////size_t localTail = tail_++;
			//size_t localTail = tail_.fetch_add(1);
			//outVal = vec_[localTail % maxSize_];
			//if (tail_ >= maxSize_)
			//	tail_ = tail_% maxSize_;
			
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			return head_.load() - tail_.load();
		}

		bool empty()
		{
			return head_.load() == tail_.load();
		}

	private:
		size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		//std::atomic<size_t> size_a;
		std::atomic<size_t> head_; //stores the index where next element will be pushed
		std::atomic<size_t> tail_; //stores the index of object which will be popped
	};
	
}