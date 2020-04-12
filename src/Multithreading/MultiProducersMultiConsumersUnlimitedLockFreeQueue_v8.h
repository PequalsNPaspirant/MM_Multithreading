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

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v6:
Modified version of v5. This version uses only two atomic variables: next and last. The first is non-atomic variable.
It makes use of first_.next_a to point to actual first element of queue.

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v7:
This version initializes first and last to null. It does not use any dummy element which is allocated at start
like all above versions.

-- MultiProducersMultiConsumersUnlimitedLockFreeQueue_v8:
Modified version of v7. This version uses only two atomic variables: next and last. The first is non-atomic variable.
It makes use of first_.next_a to point to actual first element of queue.
*/

#define CACHE_LINE_SIZE 64

namespace mm {

	void myAssert(bool expression)
	{
		if (!expression)
		{
			int *p = nullptr;
			*p = 10;
		}
	}

	template <typename T>
	class MultiProducersMultiConsumersUnlimitedLockFreeQueue_v8
	{
	private:
		struct Node
		{
			Node() : value_{}, next_a{ nullptr } { }
			Node(T&& val) : value_{ std::move(val) }, next_a{ nullptr } { }
			T value_;
			atomic<Node*> next_a;
			//Node* next_;
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) > 0 ? CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) : 1];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v8()
			: 
			//size_a{ 0 },
			queueHasOneElementAndPushOrPopInProgress_a{ false },
			consumerLock_a{ false }
		{
			first_a = last_a = nullptr;
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v8()
		{
			Node* curr = first_a.load(memory_order_seq_cst);
			while(curr != nullptr)      // release the list
			{
				Node* tmp = curr;
				curr = curr->next_a;
				delete tmp;
			}
		}

		void push(T&& obj)
		{
			Node* tmp = new Node{ std::move(obj) };

			//When queue has just one element, allow only one producer or only one consumer
			while (queueHasOneElementAndPushOrPopInProgress_a.exchange(true))
			{
			}

			Node* first = nullptr;
			Node* last = nullptr;
			first = first_a.load(memory_order_seq_cst);
			last = last_a.load(memory_order_seq_cst);

			bool holdLock = first == last;
			if (!holdLock)
				queueHasOneElementAndPushOrPopInProgress_a.store(false, memory_order_seq_cst);

			Node* oldLast = last_a.exchange(tmp, memory_order_seq_cst);

			if (oldLast)
				oldLast->next_a.store(tmp, memory_order_seq_cst); //TODO: oldLast might be deleted by consumer. Protect it! DONE: line #1 does it!
			else
				first_a.store(tmp, memory_order_seq_cst);

			if (holdLock)
				queueHasOneElementAndPushOrPopInProgress_a.store(false, memory_order_seq_cst);
		}

		//exception SAFE pop() version. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

			while (consumerLock_a.exchange(true))
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;
			}

			Node* theFirst = nullptr;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				theFirst = first_a.load(memory_order_seq_cst);

			} while (theFirst == nullptr);

			Node* theNext = nullptr;
			//bool locked = false;
			bool holdLock = false;
			//do
			//{
				//if(locked == true)
				//	queueHasOneElementAndPushOrPopInProgress_a.store(false, memory_order_seq_cst);

				//do
				//{
				//	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				//	const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				//	if (duration >= timeout)
				//		return false;

				//	theFirst = first_a.load(memory_order_seq_cst);

				//} while (theFirst == nullptr);

				//When queue has just one element, allow only one producer or only one consumer
				while (queueHasOneElementAndPushOrPopInProgress_a.exchange(true))
				{
					std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
					const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					if (duration >= timeout)
						return false;
				}

				//locked = true;
				//theFirst = first_a.load(memory_order_seq_cst);
				//theNext = theFirst ? theFirst->next_a.load(memory_order_seq_cst) : nullptr;
				theNext = theFirst->next_a.load(memory_order_seq_cst);
				Node* last = last_a.load(memory_order_seq_cst);
				//holdLock = theFirst != nullptr && theFirst == last;
				holdLock = theFirst == last;
				if (!holdLock)
				{
					//locked = false;
					queueHasOneElementAndPushOrPopInProgress_a.store(false, memory_order_seq_cst);
				}

				//theNext = theFirst->next_a.load(memory_order_seq_cst);
			//} while (theFirst == nullptr || !first_a.compare_exchange_weak(theFirst, theNext, memory_order_seq_cst));

				first_a.store(theNext);

			if (theNext == nullptr)
			{
				myAssert(holdLock);
				last_a.store(nullptr, memory_order_seq_cst);
				//last_a.compare_exchange_weak(theFirst, nullptr, memory_order_seq_cst);
			}
			outVal = std::move(theFirst->value_);
			delete theFirst;

			if (holdLock)
				queueHasOneElementAndPushOrPopInProgress_a.store(false, memory_order_seq_cst);

			consumerLock_a.store(false);

			return true;
		}

		size_t size()
		{
			//TODO: Use synchronization
			size_t size = 0;
			for (Node* curr = first_a.load(); curr != nullptr; curr = curr->next_a)      // release the list
			{
				++size;
			}

			return size;
		}

		bool empty()
		{
			//TODO: Use synchronization
			//return first_a == nullptr || (first_a != nullptr && first_a->next_a == nullptr);
			bool firstNull = first_a.load() == nullptr;
			bool lastNull = last_a.load() == nullptr;
			if (firstNull != lastNull)
			{
				int v = 0;
			}
			return firstNull;
		}

	private:
		char pad0[CACHE_LINE_SIZE];

		atomic<Node*> first_a;
		char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

		atomic<Node*> last_a;
		char pad2[CACHE_LINE_SIZE - sizeof(Node*)];

		//atomic<size_t> size_a;
		//char pad3[CACHE_LINE_SIZE - sizeof(atomic<size_t>)];

		atomic<bool> queueHasOneElementAndPushOrPopInProgress_a;
		char pad4[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		atomic<bool> consumerLock_a;
		char pad5[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		atomic<bool> producerLock_a;
		char pad6[CACHE_LINE_SIZE - sizeof(atomic<bool>)];
	};
}