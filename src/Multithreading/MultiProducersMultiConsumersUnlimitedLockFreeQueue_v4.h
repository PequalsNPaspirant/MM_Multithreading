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
			Node() : value_{}, next_ { nullptr } { }
			Node(T&& val) : value_{ std::move(val) }, next_{ nullptr } { }
			T value_;
			//atomic<Node*> next_a;
			Node* next_;
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) > 0 ? CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>) : 1];
		};

	public:
		MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4()
			: size_a{ 0 }
		{
			first_a = last_a = new Node{};
		}
		~MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4()
		{
			Node* curr = first_a.load();
			while(curr != nullptr)      // release the list
			{
				Node* tmp = curr;
				curr = curr->next_;
				delete tmp;
			}
		}

		void push(T&& obj)
		{
			Node* tmp = new Node{};
			Node* oldLast = last_a.exchange(tmp, memory_order_seq_cst);
			oldLast->value_ = std::move(obj);
			oldLast->next_ = tmp;         // publish to consumers
			++size_a;
		}

		//exception SAFE pop() version. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
			
			size_t size = 0;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				size = size_a.load(memory_order_seq_cst);
				//size_t nextSize = size - 1;
			} while (size == 0 || !size_a.compare_exchange_weak(size, size - 1, memory_order_seq_cst)); // size - 1 will be calculated before calling compare_exchange_weak(), so its safe
				
			Node* theFirst = nullptr;
			Node* theNext = nullptr;
			do
			{
				theFirst = first_a.load(memory_order_seq_cst);
				//theNext = theFirst->next_a; //theFirst can be deleted by another consumer thread at line: 'a' below
				theNext = first_a.load(memory_order_seq_cst)->next_;
			}
			while(!first_a.compare_exchange_strong(theFirst, theNext, memory_order_seq_cst));      // queue is being used by other consumer thread

			// now copy it back. If the exception is thrown at this statement, the object will be lost! 
			outVal = std::move(theFirst->value_);
			delete theFirst;      // and the old dummy // This is line: 'a'

			return true;      // and report success
		}

		size_t size()
		{
			return size_a.load(memory_order_seq_cst);
		}

		bool empty()
		{
			return size_a.load() == 0;
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

		atomic<size_t> size_a;

		char pad4[CACHE_LINE_SIZE - sizeof(atomic<size_t>)];
	};
}