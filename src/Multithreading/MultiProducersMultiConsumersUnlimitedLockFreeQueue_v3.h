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
This is a modified version of Herb Sutter's queue (reference below).
It does not implement spin locks using producerLock_a and producerLock_a. Instead it uses first and last as atomic variables and gives the sole ownership of
next available node to current thread.

Reference: Herb Sutter's blog:
https://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363?pgno=1
*/

#define CACHE_LINE_SIZE 64

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3
	{
	private:
		struct Node
		{
			Node(const T& val) : value_(val), next_a(nullptr) { }
			T value_;
			atomic<Node*> next_a;
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>)];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3()
		{
			first_a = last_a = new Node(T{});
			//producerLock_a = consumerLock_a = false;
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3()
		{
			Node* curr = first_a;
			while(curr != nullptr)      // release the list
			{
				Node* tmp = curr;
				curr = curr->next_a;
				//delete tmp->value_;       // no-op if null
				delete tmp;
			}
		}

		void push(T&& obj)
		{
			Node* tmp = new Node(obj);
/*			while (producerLock_a.exchange(true))
			{
			} */  // acquire exclusivity
			Node* old = last_a.exchange(tmp);
			old->next_a.store(tmp);         // publish to consumers
			//last_a = tmp;             // swing last forward
			//producerLock_a = false;       // release exclusivity
		}

		//exception SAFE pop() version. TODO: Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
/*			while (consumerLock_a.exchange(true))
			{
			} */   // acquire exclusivity

			Node* theFirst = nullptr;
			Node* theNext = nullptr;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				theFirst = first_a.load();
				theNext = first_a.load()->next_a;
				//if (!theNext)
				//	continue;
			}
			while(
				theNext == nullptr                                          // queue is empty
				|| !first_a.compare_exchange_strong(theFirst, theNext)      // queue is being used by other consumer thread
				);

			//Node* theFirst = first_a.load();
			//while(theFirst->next_a == nullptr || !first_a.compare_exchange_weak(theFirst, theFirst->next_a));

			
			//if (theNext != nullptr)      // if queue is nonempty
			{
				//T* val = theNext->value_;    // take it out
				outVal = theNext->value_;    // now copy it back. If the exception is thrown at this statement, the state of the entire queue will remain unchanged. but this retains lock for more time.
				//theNext->value_ = nullptr;  // of the Node
				//first_a = theNext;          // swing first forward
				//consumerLock_a = false;             // release exclusivity
													//outVal = *val;    // now copy it back here if the availability of queue i.e. locking it for least possible time is more important than exceptional neutrality. 
				//delete val;       // clean up the value_
				delete theFirst;      // and the old dummy
				return true;      // and report success
			}

			//consumerLock_a = false;   // release exclusivity
			//return false;                  // report queue was empty
		}

		size_t size()
		{
			//TODO: Use synchronization
			size_t size = 0;
			Node* curr = first_a.load()->next_a;
			for (; curr != nullptr; curr = curr->next_a)      // release the list
			{
				++size;
			}

			return size > 0 ? size - 1 : 0;
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