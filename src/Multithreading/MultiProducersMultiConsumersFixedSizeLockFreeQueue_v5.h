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
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5(size_t maxSize)
			: maxSize_(maxSize),
			size_{ 0 },
			vec_(maxSize), 
			producersLock_{ false },
			consumersLock_{ false },
			tailProducers_{ 0 },			
			tailConsumers_{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t localHead, localTail, nextLocalHead;
			bool queueFull = false;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localHead = headProducers_.load(); //Read tail value only once at the start
				nextLocalHead = (localHead + 1) % maxSize_;
				//localTail = tailProducers_.load();
				
				//if (currentHead - localTail == maxSize_)
				//	queueFull = true;
				//else if (currentHead > maxSize_)
				//	localHead %= maxSize_;
				//localHead = currentHead; // % maxSize_;
				//if (nextLocalHead == localTail) //queue will be full after inserting current element into queue
				//	nextLocalHead += maxSize_; //set it to a value offset by maxSize_, so that the condition to check whether queue is full will be true

			} while (
				size_.load() == maxSize_                         // if the queue is not full
				|| !headProducers_.compare_exchange_weak(localHead, nextLocalHead) // if some other producer thread updated head_ till now
				);
			vec_[localHead] = std::move(obj);

			do
			{

			} while (!headConsumers_.compare_exchange_weak(localHead, nextLocalHead));

			++size_;

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t localHead, localTail, nextLocalTail;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localTail = tailConsumers_.load(); //Read tail value only once at the start
				nextLocalTail = (localTail + 1) % maxSize_;
				localHead = headConsumers_.load();

			} while (
				size_.load() == 0                                      // Make sure the queue is not empty
				|| !tailConsumers_.compare_exchange_weak(localTail, nextLocalTail) // Make sure no other consumer thread updated tail_ till now
				);
			outVal = std::move(vec_[localTail]);

			do
			{

			} while (!tailProducers_.compare_exchange_weak(localTail, nextLocalTail));

			--size_;
			
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			size_t size = headConsumers_.load() - tailProducers_.load();
			//return (size < 0 ? maxSize_ - size : size);
			return size;
		}

		bool empty()
		{
			size_t size = headConsumers_.load() - tailProducers_.load();
			return size == 0;
		}

	private:
		size_t maxSize_;
		std::atomic<size_t> size_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		std::atomic<bool> producersLock_;
		std::atomic<bool> consumersLock_;
		size_t head_;
		size_t tail_;
	};
	
}