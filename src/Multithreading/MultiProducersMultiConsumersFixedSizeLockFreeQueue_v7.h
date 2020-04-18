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

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

/*
This is Multi Producers Multi Consumers Fixed Size Lock Free Queue.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1:
This is implemented using two atomic boolean flags to create spin locks for producers and consumers.
It uses another atomic variable size_a to keep track of whether queue is empty or full.
Producers and Consumers wait if the queue is empty i.e. functions push() and pop() waits if the queue is empty,
instead of returning false.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2
Modifications to v1: It does not use spin locks and size_a. Instead it uses atomic integers head_a and tail_a to
keep track of next ready element which producers and consumers can access.
Also every element has atomic bool status_a to notify producers and consumers has done processing it.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3
Modifications to v2: Instead of status_a it keeps std::atomic<T*> pObj_a and assigns it to null/valid pointer
to notify producers and consumers has done processing it.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4
It does not use anything in every element. Each element is simply T, nothing else.
It uses two copies of pair of atomic counters head and tail for producers and consumers.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5
Modifications to v4: The object T and the members of class are padded by required size to fill out cache line.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v6
Modifications to v5: simplified implementation of v5.

-- MultiProducersMultiConsumersFixedSizeLockFreeQueue_v7
It uses only two atomic counters head_a and tail_a. It keeps T* in the circular queue.
It reads the T* at current counter and then increaments the counter only if it is equal to last value read.
*/

namespace mm {

#define CACHE_LINE_SIZE 64

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v7
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v7(size_t maxSize)
			: maxSize_(maxSize),
			vec_(maxSize),
			head_a{ 0 },
			tail_a{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			T* pObj = new T{ std::move(obj) };
			size_t localHead = head_a.load(memory_order_acquire);
			size_t localTail = 0;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localTail = tail_a.load(memory_order_acquire);

			} while (!(localTail <= localHead && localHead - localTail < maxSize_)     // while the queue is full
				|| !head_a.compare_exchange_weak(localHead, localHead + 1, memory_order_seq_cst));
			
			vec_[localHead % maxSize_].obj_ = pObj; //NOTE: This should happen before or atomically with increment of head_a

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			
			size_t localHead = 0;
			size_t localTail = tail_a.load(memory_order_acquire);
			T* pCurrentObj;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				localHead = head_a.load(memory_order_acquire);
				pCurrentObj = vec_[localTail % maxSize_].obj_;

			} while (!(localTail < localHead)        // Make sure the queue is not empty
				|| !tail_a.compare_exchange_weak(localTail, localTail + 1, memory_order_seq_cst));

			outVal = std::move(*pCurrentObj);
			delete pCurrentObj;

			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			size_t size = head_a.load() - tail_a.load();
			return size;
		}

		bool empty()
		{
			size_t size = head_a.load() - tail_a.load();
			return size == 0;
		}

	private:
		const size_t maxSize_;
		char pad1[CACHE_LINE_SIZE - sizeof(size_t)];

		struct Data
		{
			T* obj_;
			char pad[CACHE_LINE_SIZE - sizeof(T*)];
		};
		std::vector<Data> vec_; //This will be used as ring buffer / circular queue
		char pad2[CACHE_LINE_SIZE - sizeof(std::vector<Data>)];

		std::atomic<size_t> head_a; //stores the index where next element will be pushed/produced
		char pad3[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		//std::atomic<size_t> headConsumers_a; //stores the index where next element will be pushed/produced - published to consumers
		//char pad4[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		std::atomic<size_t> tail_a; //stores the index of object which will be popped/consumed - published to producers
		char pad5[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		//std::atomic<size_t> tailConsumers_a; //stores the index of object which will be popped/consumed
		//char pad6[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];
	};

}