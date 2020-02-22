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
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3(size_t maxSize)
			: maxSize_(maxSize),
			vec_(maxSize),
			head_{ 0 },
			tail_{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			size_t localHead = head_.fetch_add(1, memory_order_seq_cst) % maxSize_;
			T* ptr = new T{ std::move(obj) };
			T* expectedPtr = nullptr;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				expectedPtr = nullptr;
			} while (!vec_[localHead].pObj_.compare_exchange_weak(expectedPtr, ptr, memory_order_seq_cst));  //Make sure this slot in queue is not already occupied

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			size_t localTail = tail_.fetch_add(1, memory_order_seq_cst) % maxSize_;
			T* ptr = nullptr;
			while((ptr = vec_[localTail].pObj_.exchange(nullptr, memory_order_seq_cst)) == nullptr)
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
			size_t size = head_.load() - tail_.load();
			return size;
		}

		bool empty()
		{
			size_t size = head_.load() - tail_.load();
			return size == 0;
		}

	private:
		size_t maxSize_;
		struct Data
		{
			Data()
				: pObj_{ nullptr }
			{}

			std::atomic<T*> pObj_;
			char pad[CACHE_LINE_SIZE - sizeof(std::atomic<T*>)];
		};
		std::vector<Data> vec_; //This will be used as ring buffer / circular queue
		std::atomic<size_t> head_; //stores the index where next element will be pushed/produced
		std::atomic<size_t> tail_; //stores the index where next element will be pushed/produced - published to consumers
	};

}