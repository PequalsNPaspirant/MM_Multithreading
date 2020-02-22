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
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4(size_t maxSize)
			: maxSize_(maxSize),
			vec_(maxSize),
			headProducers_{ 0 },
			headConsumers_{ 0 },
			tailProducers_{ 0 },
			tailConsumers_{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t localHead, localTail;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localHead = headProducers_.load(memory_order_seq_cst); //Read tail value only once at the start
				localTail = tailProducers_.load(memory_order_seq_cst);

			} while (
				!(localTail <= localHead && localHead - localTail < maxSize_)                         // if the queue is not full
				|| !headProducers_.compare_exchange_weak(localHead, localHead + 1, memory_order_seq_cst) // if some other producer thread updated head_ till now
				);

			vec_[localHead % maxSize_] = std::move(obj);

			size_t expected = localHead;
			do 
			{
				expected = localHead;
			} while (!headConsumers_.compare_exchange_weak(expected, localHead + 1, memory_order_seq_cst));

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t localHead, localTail;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localTail = tailConsumers_.load(memory_order_seq_cst); //Read tail value only once at the start
				localHead = headConsumers_.load(memory_order_seq_cst);

			} while (
				!(localTail < localHead)                                      // Make sure the queue is not empty
				|| !tailConsumers_.compare_exchange_weak(localTail, localTail + 1, memory_order_seq_cst) // Make sure no other consumer thread updated tail_ till now
				);

			outVal = std::move(vec_[localTail % maxSize_]);

			size_t expected = localTail;
			do 
			{
				expected = localTail;
			} while (!tailProducers_.compare_exchange_weak(expected, localTail + 1, memory_order_seq_cst));

			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			size_t size = headConsumers_.load() - tailProducers_.load();
			return size;
		}

		bool empty()
		{
			size_t size = headConsumers_.load() - tailProducers_.load();
			return size == 0;
		}

	private:
		size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		std::atomic<size_t> headProducers_; //stores the index where next element will be pushed/produced
		std::atomic<size_t> headConsumers_; //stores the index where next element will be pushed/produced - published to consumers
		std::atomic<size_t> tailProducers_; //stores the index of object which will be popped/consumed - published to producers		
		std::atomic<size_t> tailConsumers_; //stores the index of object which will be popped/consumed
	};
	
}