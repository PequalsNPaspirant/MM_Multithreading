#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <cassert> //for assert()
#include <cmath>
#include <atomic>
#include <chrono>
using namespace std;

/*
This is Multi Producers Multi Consumers Unlimited Size Lock Free Queue.

Reference: Herb Sutter's blog:
https://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363?pgno=1

This is implemented using two atomic boolean flags to create spin locks for producers and consumers.
It uses its own forward list implementation having atomic next ptr in each node.
The list stores T* and uses two dynamic allocations to allocate memory for node and also for data.
Producers never need to wait because its unlimited queue and will never be full.
Consumers have to wait and retry on their own because it returns false if the queue is empty.

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1:
Modifications to original implementation (original code is commented):
Consumers wait if the queue is empty i.e. function pop() waits if the queue is empty,
instead of returning false.

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v2:
The list stores T (instead of T*) and uses ONLY ONE dynamic allocation to allocate memory for node.

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3:
It does not implement spin locks using producerLock_a and producerLock_a. Instead it uses first and last as atomic variables and gives the sole ownership of
next available node to current thread. This algo uses total 3 atomic variables: first, last and next

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4:
Modified version of v3. This version uses only two atomic variables: next and last. The first is non-atomic variable.
It makes use of first_.next_a to point to actual first element of queue.

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v5:
Modified version of v3. It blocks all other consumer threads until one consumer thread gets
the ownership of first node in queue and makes first point to the next element in queue.
*/

#define CACHE_LINE_SIZE 64

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersUnlimitedLockFreeQueue_v5
	{
	private:
		struct Node
		{
			Node() : value_{}, next_a { nullptr } { }
			Node(T&& val) : value_{ std::move(val) }, next_a{ nullptr } { }
			T value_;
			atomic<Node*> next_a; //TODO: Check if we can use non-atomic variable next_ here
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) > 0 ? CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) : 1];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v5()
		{
			first_a = last_a = new Node{};
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v5()
		{
			Node* curr = first_a.load();
			while(curr != nullptr)      // release the list
			{
				Node* tmp = curr;
				curr = curr->next_a;
				delete tmp;
			}
		}

		void push(T&& obj)
		{
			Node* tmp = new Node{};
			Node* oldLast = last_a.exchange(tmp, memory_order_seq_cst);
			oldLast->value_ = std::move(obj);
			oldLast->next_a.store(tmp, memory_order_seq_cst);         // publish to consumers
		}

		//exception SAFE pop() version. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			Node* theFirst = nullptr;
			Node* theNext = nullptr;

			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				theFirst = first_a.exchange(nullptr, memory_order_seq_cst);

			} while (theFirst == nullptr);

			//Only one thread is guaranteed after this line until line#1 below
			//Also theFirst is never same for different concurrent threads accessing code after line#1, so delete theFirst is always safe
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				//theFirst = first_a.load(memory_order_seq_cst);
				//theNext = theFirst->next_a; //theFirst can be deleted by another consumer thread at line#2 below
				theNext = theFirst->next_a.load(memory_order_seq_cst);

			} while (
				theNext == nullptr                                                             // if the queue is empty
				//|| !first_a.compare_exchange_weak(theFirst, theNext, memory_order_seq_cst)     // or if the queue is being used by another consumer thread
				);

			//theFirst = first_a.load(memory_order_seq_cst);
			first_a.store(theNext, memory_order_seq_cst); // This is line#1
			//theFirst = first_a.exchange(theNext, memory_order_seq_cst); // This is line#1

			// now copy it back. If the exception is thrown at this statement, the object will be lost! 
			outVal = std::move(theFirst->value_);
			delete theFirst;      // This is line#2

			return true;      // and report success
		}

		size_t size()
		{
			//TODO: Use synchronization
			size_t size = 0;
			for (Node* curr = first_a.load()->next_a; curr != nullptr; curr = curr->next_a)      // release the list
			{
				++size;
			}

			return size;
		}

		bool empty()
		{
			//TODO: Use synchronization
			//return first_a == nullptr || (first_a != nullptr && first_a->next_a == nullptr);
			return first_a.load()->next_a == nullptr;
		}

	private:
		char pad0[CACHE_LINE_SIZE];

		// for one consumer at a time
		atomic<Node*> first_a;
		char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among consumers
		//atomic<bool> consumerLock_a;
		//char pad2[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		// for one producer at a time
		atomic<Node*> last_a;
		char pad3[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among producers
		//atomic<bool> producerLock_a;
		//char pad4[CACHE_LINE_SIZE - sizeof(atomic<bool>)];
	};
}