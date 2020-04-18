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
*/

namespace mm {

#define CACHE_LINE_SIZE 64

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2(size_t maxSize)
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
			Status expected = Status::empty;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				expected = Status::empty;
			} while (!vec_[localHead].status_a.compare_exchange_weak(expected, Status::intermediate, memory_order_seq_cst));  //Make sure this slot in queue is not already occupied

			vec_[localHead].obj_ = std::move(obj);
			vec_[localHead].status_a.store(Status::filled, memory_order_seq_cst);

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			size_t localTail = tail_a.fetch_add(1, memory_order_seq_cst) % maxSize_;
			Status expected = Status::filled;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				expected = Status::filled;
			} while (!vec_[localTail].status_a.compare_exchange_weak(expected, Status::intermediate, memory_order_seq_cst)); //Block if this slot in queue is not filled yet

			outVal = std::move(vec_[localTail].obj_);
			vec_[localTail].status_a.store(Status::empty, memory_order_seq_cst);

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
		enum class Status
		{
			intermediate = 0,
			empty,
			filled
		};
		struct Data
		{
			Data()
				: status_a{ Status::empty }
			{}

			T obj_;
			std::atomic<Status> status_a;
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(std::atomic<Status>)];
		};
		std::vector<Data> vec_; //This will be used as ring buffer / circular queue
		std::atomic<size_t> head_a; //stores the index where next element will be pushed/produced
		std::atomic<size_t> tail_a; //stores the index where next element will be pushed/produced - published to consumers
	};

}