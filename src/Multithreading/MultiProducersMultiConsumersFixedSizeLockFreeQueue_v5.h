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
			size_a{ 0 },
			vec_(maxSize), 
			producerLock_a{ false },
			consumerLock_a{ false },
			head_{ 0 },
			tail_{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			while (producerLock_a.exchange(true))
			{
			}   // acquire exclusivity

			while (size_a.load(memory_order_seq_cst) == maxSize_)
			{
			}

			vec_[head_] = std::move(obj);
			head_ = (head_ + 1) % maxSize_;
			++size_a;
			producerLock_a = false;       // release exclusivity

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;
			return true;
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			while (consumerLock_a.exchange(true))
			{
			}    // acquire exclusivity

			while (size_a.load(memory_order_seq_cst) == 0)
			{
			}

			outVal = std::move(vec_[tail_]);
			tail_ = (tail_ + 1) % maxSize_;
			--size_a;			
			consumerLock_a = false;             // release exclusivity

			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			return size_a.load(memory_order_seq_cst);
		}

		bool empty()
		{
			return size_a.load(memory_order_seq_cst) == 0;
		}

	private:
		size_t maxSize_;
		std::atomic<size_t> size_a;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		std::atomic<bool> producerLock_a;
		std::atomic<bool> consumerLock_a;
		size_t head_;
		size_t tail_;
	};
	
}