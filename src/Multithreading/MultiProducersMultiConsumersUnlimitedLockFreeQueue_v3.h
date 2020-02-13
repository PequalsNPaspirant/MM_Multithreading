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
			Node() : value_{}, next_a { nullptr } { }
			Node(T&& val) : value_{ std::move(val) }, next_a{ nullptr } { }
			T value_;
			atomic<Node*> next_a;
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) > 0 ? CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) : 1];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3()
		{
			first_a = last_a = new Node{};
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3()
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

				//
				//theNext = first_a.load()->next_a;
				//theNext = theFirst->next_a;
				do
				{
					theFirst = first_a.load(memory_order_seq_cst);
					theNext = theFirst->next_a;
				} while (theNext == nullptr);
			}
			while(
				!first_a.compare_exchange_strong(theFirst, theNext, memory_order_seq_cst)      // queue is being used by other consumer thread
				);
				
			{
				int n = theFirst->value_.getValue();
				const string& str = theFirst->value_.getStr();
				if ((n % 256) != str.length())
				{
					int  x = 0;
				}
			}

				// now copy it back. If the exception is thrown at this statement, the state of the entire queue will remain unchanged. 
				// but this makes unnecessary copy of object if it fails further in compare_exchange_weak().
				outVal = std::move(theFirst->value_);
				delete theFirst;      // and the old dummy

				int n = outVal.getValue();
				const string& str = outVal.getStr();
				if ((n % 256) != str.length())
				{
					int  x = 0;
				}

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