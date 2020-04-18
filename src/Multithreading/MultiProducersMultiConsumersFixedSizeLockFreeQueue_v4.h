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
*/

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4(size_t maxSize)
			: maxSize_(maxSize),
			vec_(maxSize),
			headProducers_a{ 0 },
			headConsumers_a{ 0 },
			tailProducers_a{ 0 },
			tailConsumers_a{ 0 }
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

				localHead = headProducers_a.load(memory_order_seq_cst); //Read tail value only once at the start
				localTail = tailProducers_a.load(memory_order_seq_cst);

			} while (
				!(localTail <= localHead && localHead - localTail < maxSize_)                         // if the queue is not full
				|| !headProducers_a.compare_exchange_weak(localHead, localHead + 1, memory_order_seq_cst) // if some other producer thread updated head_ till now
				);

			vec_[localHead % maxSize_] = std::move(obj);

			size_t expected = localHead;
			do 
			{
				expected = localHead;
			} while (!headConsumers_a.compare_exchange_weak(expected, localHead + 1, memory_order_seq_cst));

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

				localTail = tailConsumers_a.load(memory_order_seq_cst); //Read tail value only once at the start
				localHead = headConsumers_a.load(memory_order_seq_cst);

			} while (
				!(localTail < localHead)                                      // Make sure the queue is not empty
				|| !tailConsumers_a.compare_exchange_weak(localTail, localTail + 1, memory_order_seq_cst) // Make sure no other consumer thread updated tail_ till now
				);

			outVal = std::move(vec_[localTail % maxSize_]);

			size_t expected = localTail;
			do 
			{
				expected = localTail;
			} while (!tailProducers_a.compare_exchange_weak(expected, localTail + 1, memory_order_seq_cst));

			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			size_t size = headConsumers_a.load() - tailProducers_a.load();
			return size;
		}

		bool empty()
		{
			size_t size = headConsumers_a.load() - tailProducers_a.load();
			return size == 0;
		}

	private:
		const size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		std::atomic<size_t> headProducers_a; //stores the index where next element will be pushed/produced
		std::atomic<size_t> headConsumers_a; //stores the index where next element will be pushed/produced - published to consumers
		std::atomic<size_t> tailProducers_a; //stores the index of object which will be popped/consumed - published to producers		
		std::atomic<size_t> tailConsumers_a; //stores the index of object which will be popped/consumed
	};
	
}