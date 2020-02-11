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
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2(size_t maxSize)
			: maxSize_(maxSize), 
			vec_(maxSize), 
			head1_{ 0 },
			tail1_{ 0 },
			head2_{ 0 },
			tail2_{ 0 }
		{
		}

		bool push(T&& obj, const std::chrono::milliseconds& timeout = std::chrono::milliseconds{ 1000 * 60 * 60 }) //default timeout = 1 hr
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			size_t currentHead, localHead, localTail, nextLocalHead;
			bool queueFull = false;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				currentHead = head1_.load(); //Read tail value only once at the start
				nextLocalHead = (currentHead + 1) % maxSize_;
				localTail = tail2_.load();
				
				//if (currentHead - localTail == maxSize_)
				//	queueFull = true;
				//else if (currentHead > maxSize_)
				//	localHead %= maxSize_;
				localHead = currentHead % maxSize_;
				if (nextLocalHead == localTail) //queue will be full after inserting current element into queue
					nextLocalHead += maxSize_; //set it to a value offset by maxSize_, so that the condition to check whether queue is full will be true

			} while (
				currentHead - localTail == maxSize_                         // if the queue is not full
				|| !head1_.compare_exchange_weak(currentHead, nextLocalHead) // if some other producer thread updated head_ till now
				);
			vec_[localHead] = std::move(obj);

			do
			{

			} while (!head2_.compare_exchange_weak(currentHead, nextLocalHead));

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

				localTail = tail1_.load(); //Read tail value only once at the start
				nextLocalTail = (localTail + 1) % maxSize_;
				localHead = head2_.load();

			} while (
				localHead == localTail                                      // Make sure the queue is not empty
				|| !tail1_.compare_exchange_weak(localTail, nextLocalTail) // Make sure no other consumer thread updated tail_ till now
				);
			outVal = std::move(vec_[localTail]);

			do
			{

			} while (!tail2_.compare_exchange_weak(localTail, nextLocalTail));
			
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return true;
		}

		size_t size()
		{
			return head2_.load() - tail2_.load();
		}

		bool empty()
		{
			return head2_.load() == tail2_.load();
		}

	private:
		size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		std::atomic<size_t> head1_; //stores the index where next element will be pushed
		std::atomic<size_t> tail1_; //stores the index of object which will be popped
		std::atomic<size_t> head2_; //stores the index where next element will be pushed
		std::atomic<size_t> tail2_; //stores the index of object which will be popped
	};
	
}