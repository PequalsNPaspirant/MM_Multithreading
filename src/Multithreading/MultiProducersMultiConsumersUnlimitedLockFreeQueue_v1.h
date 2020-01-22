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
			Node(T* val) : value(val), next(nullptr) { }
			T* value;
			atomic<Node*> next;
			char pad[CACHE_LINE_SIZE - sizeof(T*) - sizeof(atomic<Node*>)];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1() 
		{
			first = last = new Node(nullptr);
			producerLock = consumerLock = false;
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1() 
		{
			while (first != nullptr)      // release the list
			{
				Node* tmp = first;
				first = tmp->next;
				delete tmp->value;       // no-op if null
				delete tmp;
			}
		}

		void push(T&& obj)
		{
			Node* tmp = new Node(new T(obj));
			while (producerLock.exchange(true))
			{
			}   // acquire exclusivity
			last->next = tmp;         // publish to consumers
			last = tmp;             // swing last forward
			producerLock = false;       // release exclusivity
		}

		//exception UNSAFE pop() version
		T pop()
		{
			while (consumerLock.exchange(true))
			{
			}    // acquire exclusivity
			Node* theFirst = first;
			Node* theNext = first->next;
			if (theNext != nullptr)      // if queue is nonempty
			{
				T* val = theNext->value;    // take it out
				theNext->value = nullptr;  // of the Node
				first = theNext;          // swing first forward
				consumerLock = false;             // release exclusivity
				T result = *val;    // now copy it back
				delete val;       // clean up the value
				delete theFirst;      // and the old dummy
				return result;      // and report success
			}

			consumerLock = false;   // release exclusivity
			return T();                  // report queue was empty
		}

		//exception SAFE pop() version
		bool pop(T& outVal)
		{
			while (consumerLock.exchange(true))
			{
			}    // acquire exclusivity
			Node* theFirst = first;
			Node* theNext = first->next;
			if (theNext != nullptr)      // if queue is nonempty
			{   
				T* val = theNext->value;    // take it out
				outVal = *val;    // now copy it back. If the exception is thrown at this statement, the state of the entire queue is not changed.
				theNext->value = nullptr;  // of the Node
				first = theNext;          // swing first forward
				consumerLock = false;             // release exclusivity
				//outVal = *val;    // now copy it back. Do this copying above to make this function exception neutral.
				delete val;       // clean up the value
				delete theFirst;      // and the old dummy
				return true;      // and report success
			}
			consumerLock = false;   // release exclusivity
			return false;                  // report queue was empty
		}

		//pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			//TODO: implement the timeout using sleep and (atomic?) counter
			return true;
		}

		size_t size()
		{
			size_t size = 0;
			for (; first != nullptr; first = first->next)      // release the list
			{
				++size;
			}

			return size - 1;
		}

		bool empty()
		{
			return first == nullptr || (first != nullptr && first->next == nullptr);
		}

	private:
		char pad0[CACHE_LINE_SIZE];

		// for one consumer at a time
		Node* first;

		char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among consumers
		atomic<bool> consumerLock;

		char pad2[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		// for one producer at a time
		Node* last;

		char pad3[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among producers
		atomic<bool> producerLock;

		char pad4[CACHE_LINE_SIZE - sizeof(atomic<bool>)];
	};
}