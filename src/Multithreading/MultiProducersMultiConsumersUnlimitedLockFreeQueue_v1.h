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
using namespace std;

#define CACHE_LINE_SIZE 64

/*
This is Multi Producers Multi Consumers Unlimited Size Lock Free Queue.
This is implemented using two atomic boolean flags to create spin locks for producers and consumers.
It uses its own forward list implementation having atomic next ptr in each node. 
The list stores T* and uses two dynamic allocations to allocate memory for node and also for data.
Producers never need to wait because its unlimited queue and will never be full.
Consumers have to wait and retry on their own because it returns false if the queue is empty.

Reference: Herb Sutter's blog:
https://www.drdobbs.com/parallel/writing-a-generalized-concurrent-queue/211601363?pgno=1
*/

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1
	{
	private:
		struct Node 
		{
			Node(T* val) : value(val), next_a(nullptr) { }
			T* value;
			atomic<Node*> next_a;
			char pad[CACHE_LINE_SIZE - sizeof(T*) - sizeof(atomic<Node*>)];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1() 
		{
			first_ = last_ = new Node(nullptr);
			producerLock_a = consumerLock_a = false;
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1() 
		{
			while (first_ != nullptr)      // release the list
			{
				Node* tmp = first_;
				first_ = tmp->next_a;
				delete tmp->value;       // no-op if null
				delete tmp;
			}
		}

		void push(T&& obj)
		{
			Node* tmp = new Node(new T(obj));
			while (producerLock_a.exchange(true))
			{
			}   // acquire exclusivity
			last_->next_a = tmp;         // publish to consumers
			last_ = tmp;             // swing last forward
			producerLock_a = false;       // release exclusivity
		}

		//exception SAFE pop() version. TODO: Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			while (consumerLock_a.exchange(true))
			{
			}    // acquire exclusivity
			Node* theFirst = first_;
			Node* theNext = first_->next_a;
			if (theNext != nullptr)      // if queue is nonempty
			{   
				T* val = theNext->value;    // take it out
				outVal = *val;    // now copy it back. If the exception is thrown at this statement, the state of the entire queue will remain unchanged. but this retains lock for more time.
				theNext->value = nullptr;  // of the Node
				first_ = theNext;          // swing first forward
				consumerLock_a = false;             // release exclusivity
				//outVal = *val;    // now copy it back here if the availability of queue i.e. locking it for least possible time is more important than exceptional neutrality. 
				delete val;       // clean up the value
				delete theFirst;      // and the old dummy
				return true;      // and report success
			}

			consumerLock_a = false;   // release exclusivity
			return false;                  // report queue was empty
		}

		size_t size()
		{
			//TODO: Use synchronization
			size_t size = 0;
			for (; first_ != nullptr; first_ = first_->next_a)      // release the list
			{
				++size;
			}

			return size > 0 ? size - 1 : 0;
		}

		bool empty()
		{
			//TODO: Use synchronization
			return first_ == nullptr || (first_ != nullptr && first_->next_a == nullptr);
		}

	private:
		char pad0[CACHE_LINE_SIZE];

		// for one consumer at a time
		Node* first_;

		char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among consumers
		atomic<bool> consumerLock_a;

		char pad2[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		// for one producer at a time
		Node* last_;

		char pad3[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among producers
		atomic<bool> producerLock_a;

		char pad4[CACHE_LINE_SIZE - sizeof(atomic<bool>)];
	};
}