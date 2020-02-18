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
	class MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4
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
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4()
			//: size_a{ 0 }
		{
			first_a = last_a = nullptr;
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4()
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
			Node* oldLast = last_a.exchange(tmp, memory_order_seq_cst);
			if(oldLast)
				oldLast->next_a.store(tmp, memory_order_seq_cst); //TODO: oldLast might be deleted by consumer. Protect it! DONE: line #1 does it!
			else
			{
				//Either this is initial condition i.e. first_a = last_a = nullptr
				//OR the queue has just one element and pop() operation is in progress, so wait for its completion
				Node* expected = nullptr;
				do
				{
					expected = nullptr;
				} while (!first_a.compare_exchange_weak(expected, tmp, memory_order_seq_cst));
			}

		}

		//exception SAFE pop() version. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();			
			Node* theFirst = nullptr;
			Node* theNext = nullptr;
			bool reloop = false;
			do
			{
				do
				{
					std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
					const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					if (duration >= timeout)
						return false;

					theFirst = first_a.load(memory_order_seq_cst);
					
				} while (theFirst == nullptr);

				//If theFirst & last_a are same, the queue has just one element and is now going to be popped out of queue
				Node* expected = theFirst;
				bool queueHasJustOneElement = last_a.compare_exchange_weak(expected, nullptr, memory_order_seq_cst);
				theNext = theFirst->next_a.load(memory_order_seq_cst);
				//If there are more than one elements in queue, we can not expect theNext to be nullptr
				//If there are more than one elements in queue, and theNext is nullptr
				// that means push operation for a second element is in midway, so lets wait for its completion
				reloop = !queueHasJustOneElement && theNext == nullptr;  //line #1
			} while (reloop
				|| !first_a.compare_exchange_weak(theFirst, theNext, memory_order_seq_cst));
			
			//At this time, theFirst is not a part of queue and consumer has got exclusive right to use it
			outVal = std::move(theFirst->value_);
			delete theFirst;

			return true;      // and report success
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

		// for one consumer at a time
		atomic<Node*> first_a;
		//Node* first_;
		char pad1[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among consumers
		//atomic<bool> consumerLock_a;
		//char pad2[CACHE_LINE_SIZE - sizeof(atomic<bool>)];

		// for one producer at a time
		atomic<Node*> last_a;
		char pad3[CACHE_LINE_SIZE - sizeof(Node*)];

		// shared among producers
		//atomic<bool> producerLock_a;
		//atomic<size_t> size_a;
		//char pad4[CACHE_LINE_SIZE - sizeof(atomic<size_t>)];
	};
}