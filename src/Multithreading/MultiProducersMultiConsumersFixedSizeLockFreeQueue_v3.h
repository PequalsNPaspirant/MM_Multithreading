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
*/

namespace mm {

#define CACHE_LINE_SIZE 64

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3(size_t maxSize)
			: maxSize_(maxSize),
			vec_(maxSize),
			head_a{ 0 },
			tail_a{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			size_t localHead = head_a.fetch_add(1, memory_order_seq_cst) % maxSize_;
			T* ptr = new T{ std::move(obj) };
			T* expectedPtr = nullptr;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				expectedPtr = nullptr;
			} while (!vec_[localHead].pObj_a.compare_exchange_weak(expectedPtr, ptr, memory_order_seq_cst));  //Make sure this slot in queue is not already occupied

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			size_t localTail = tail_a.fetch_add(1, memory_order_seq_cst) % maxSize_;
			T* ptr = nullptr;
			while((ptr = vec_[localTail].pObj_a.exchange(nullptr, memory_order_seq_cst)) == nullptr)
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;
			}

			outVal = std::move(*ptr);
			delete ptr;

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
			Data()
				: pObj_a{ nullptr }
			{}

			std::atomic<T*> pObj_a;
			char pad[CACHE_LINE_SIZE - sizeof(std::atomic<T*>)];
		};
		std::vector<Data> vec_; //This will be used as ring buffer / circular queue
		char pad2[CACHE_LINE_SIZE - sizeof(std::vector<Data>)];

		std::atomic<size_t> head_a; //stores the index where next element will be pushed/produced
		char pad3[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

		std::atomic<size_t> tail_a; //stores the index where next element will be pushed/produced - published to consumers
		char pad4[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

	};

}