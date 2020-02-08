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
			first_ = last_ = new Node(T{});
			//producerLock_a = consumerLock_a = false;
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3()
		{
			Node* curr = first_;
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
			Node* old = last_.exchange(tmp);
			old->next_a.store(tmp);         // publish to consumers
			//last_ = tmp;             // swing last forward
			//producerLock_a = false;       // release exclusivity
		}

		//exception SAFE pop() version. TODO: Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
/*			while (consumerLock_a.exchange(true))
			{
			} */   // acquire exclusivity

			Node* theFirst = nullptr;
			Node* theNext = nullptr;
			do
			{
				theFirst = first_.load();
				theNext = first_.load()->next_a;
				//if (!theNext)
				//	continue;
			}
			while(theNext == nullptr || !first_.compare_exchange_strong(theFirst, theNext));

			//Node* theFirst = first_.load();
			//while(theFirst->next_a == nullptr || !first_.compare_exchange_weak(theFirst, theFirst->next_a));

			
			//if (theNext != nullptr)      // if queue is nonempty
			{
				//T* val = theNext->value_;    // take it out
				outVal = theNext->value_;    // now copy it back. If the exception is thrown at this statement, the state of the entire queue will remain unchanged. but this retains lock for more time.
				//theNext->value_ = nullptr;  // of the Node
				//first_ = theNext;          // swing first forward
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
			Node* curr = first_.load()->next_a;
			for (; curr != nullptr; curr = curr->next_a)      // release the list
			{
				++size;
			}

			return size > 0 ? size - 1 : 0;
		}

		bool empty()
		{
			//TODO: Use synchronization
			//return first_ == nullptr || (first_ != nullptr && first_->next_a == nullptr);
			return first_.load()->next_a == nullptr;
		}

	private:
		char pad0[CACHE_LINE_SIZE];

		// for one consumer at a time
		atomic<Node*> first_;

		char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among consumers
		//atomic<bool> consumerLock_a;

		//char pad2[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		// for one producer at a time
		atomic<Node*> last_;

		char pad3[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among producers
		//atomic<bool> producerLock_a;

		//char pad4[CACHE_LINE_SIZE - sizeof(atomic<bool>)];
	};
}