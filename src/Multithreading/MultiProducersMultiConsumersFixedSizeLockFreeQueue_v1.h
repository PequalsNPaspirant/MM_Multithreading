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
*/

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1(size_t maxSize)
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
		const size_t maxSize_;
		std::atomic<size_t> size_a;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		std::atomic<bool> producerLock_a;
		std::atomic<bool> consumerLock_a;
		size_t head_;
		size_t tail_;
	};
	
}