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

#define CACHE_LINE_SIZE 64

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5(size_t maxSize)
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

			size_t localHead = headProducers_.fetch_add(1, memory_order_seq_cst);
			size_t localTail = 0;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localTail = tailProducers_.load(memory_order_seq_cst);

			} while (!(localTail <= localHead && localHead - localTail < maxSize_));     // while the queue is full

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

			size_t localTail = tailConsumers_.fetch_add(1, memory_order_seq_cst);
			size_t localHead = 0;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localHead = headConsumers_.load(memory_order_seq_cst);

			} while (!(localTail < localHead));        // Make sure the queue is not empty

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
		char pad1[CACHE_LINE_SIZE - sizeof(size_t)];

		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		char pad2[CACHE_LINE_SIZE - sizeof(std::vector<T>)];

		std::atomic<size_t> headProducers_; //stores the index where next element will be pushed/produced
		char pad3[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		std::atomic<size_t> headConsumers_; //stores the index where next element will be pushed/produced - published to consumers
		char pad4[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		std::atomic<size_t> tailProducers_; //stores the index of object which will be popped/consumed - published to producers	
		char pad5[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		std::atomic<size_t> tailConsumers_; //stores the index of object which will be popped/consumed
		char pad6[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];
	};

}