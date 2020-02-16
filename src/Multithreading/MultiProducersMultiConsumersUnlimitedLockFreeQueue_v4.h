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
				oldLast->next_a.store(tmp, memory_order_seq_cst);
			else
			{
				//Either this is initial condition i.e. first_a = last_a = nullptr
				//OR the queue has just one element and pop() operation is in progress, so wait for its completion
				Node* expected = nullptr;
				do
				{

				} while (!first_a.compare_exchange_weak(expected, tmp, memory_order_seq_cst));
			}
				

			//Node* expected;
			////Check if queue is empty
			//expected = nullptr;
			//if (first_a.compare_exchange_weak(expected, tmp, memory_order_seq_cst))
			//	return;

			////if there is just one element in queue, block consumer threads until push is done 
			//expected = oldLast;
			//if(first_a.compare_exchange_weak(expected, nullptr, memory_order_seq_cst))
			//{
			//	if (oldLast)
			//		oldLast->next_a.store(tmp, memory_order_seq_cst);
			//	first_a.store(oldLast, memory_order_seq_cst); //restore the value of first_a back if its still nullptr
			//}
			//else
			//{
			//	if (oldLast)
			//		oldLast->next_a.store(tmp, memory_order_seq_cst);
			//}

			{
				//if(oldLast == nullptr) //the queue is empty
				//	first_a.store(tmp, memory_order_seq_cst);
				//else
				//	oldLast->next_a.store(tmp, memory_order_seq_cst);
				

				////If oldLast is still valid i.e. it's not deleted i.e. first_a is not nullptr
				//if(first_a.load(memory_order_seq_cst) != nullptr)
				//	//At this time, consumer may delete one or more elements from queue including oldLast
					//oldLast->next_a.store(tmp, memory_order_seq_cst);

				//Node* theFirst = nullptr;
				//Node* theNext = nullptr;
				//do
				//{
				//	theFirst = first_a.load(memory_order_seq_cst);
				//	if(theFirst)
				//		theNext = theFirst->next_a.load(memory_order_seq_cst);
				//}while(theFirst != nullptr
				//	&& theFirst->next_a.load)

				//oldLast may be deleted if queue has just one element and a consumer pops it...
				//Node* expectedFirst = oldLast;
				//bool success = first_a.compare_exchange_weak(expectedFirst, nullptr, memory_order_seq_cst); //Check if queue has just one element
				//if (success) //The first_a is same as oldLast i.e. the queue has just one element and no consumer thread has not even started pop operation. first_a is set to nullptr, so all the consumers threads will be blocked if they are trying to call pop().
				//{
				//	oldLast->next_a.store(tmp, memory_order_seq_cst);
				//	first_a.store(oldLast, memory_order_seq_cst); //restore first_a back to old value
				//}
				//else //first_a is NOT same as oldLast
				//{
				//	if (expectedFirst == nullptr) //If the first_a is null, that means, consumers have popped all the elements from queue and queue is empty now
				//		first_a.store(tmp, memory_order_seq_cst);
				//	else //If the first_a is NOT null, that means, the queue has more than one elements
				//		//At this time, consumer may delete one or more elements from queue including oldLast
				//		oldLast->next_a.store(tmp, memory_order_seq_cst);
				//}
			}
		}

		//exception SAFE pop() version. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();			
			Node* theFirst = nullptr;
			Node* theNext = nullptr;
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
				bool success = last_a.compare_exchange_weak(expected, nullptr, memory_order_seq_cst);

				theNext = theFirst->next_a.load(memory_order_seq_cst);
			} while (!first_a.compare_exchange_weak(theFirst, theNext, memory_order_seq_cst));
			
			//At this time, theFirst is not a part of queue and consumer has got exclusive right to use it
			outVal = std::move(theFirst->value_);
			delete theFirst;

			

			//the statements below this line can be in any order, because they act only on local variable 'theFirst'
			//Multiple threads can come here only when:
			//    at this point, the queue is empty and producer pushes first element and changes first_a to some non-nullptr value 
			//    which is same as last_a. But its safe as it acts only on local variable 'theFirst'
			//outVal = std::move(theFirst->value_);
			//Node* theNext = theFirst->next_a;
			//the statements above this line can be in any order

			//If first_a is not changed yet by any producer thread, then set it to whatever theNext is
			//Node* expected = nullptr;
			//first_a.compare_exchange_weak(expected, theNext, memory_order_seq_cst);
			//delete theFirst;

/*			size_t size = 0;
			size_t newSize = 0;
			do
			{
				std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
				const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				if (duration >= timeout)
					return false;

				size = size_a.load(memory_order_seq_cst);
				newSize = size - 1;
			} while (size == 0 || !size_a.compare_exchange_weak(size, newSize, memory_order_seq_cst)); // size - 1 will be calculated before calling compare_exchange_weak(), so its safe
			*/

			//Node* theFirst = nullptr;
			//Node* theNext = nullptr;
			//do
			//{
			//	theFirst = first_a.load(memory_order_seq_cst);
			//	//theNext = theFirst->next_a; //theFirst can be deleted by another consumer thread at line: 'a' below
			//	theNext = first_a.load(memory_order_seq_cst)->next_;
			//}
			//while(!first_a.compare_exchange_strong(theFirst, theNext, memory_order_seq_cst));      // queue is being used by other consumer thread

			//Node* theFirst = first_a.load(memory_order_seq_cst);
			//theNext = theFirst->next_a; //theFirst can be deleted by another consumer thread at line: 'a' below
			//Node* theNext = first_a.load(memory_order_seq_cst)->next_;
			//Node* theFirst = first_a.exchange(theNext, memory_order_seq_cst);

			// now copy it back. If the exception is thrown at this statement, the object will be lost! 
			//outVal = std::move(theFirst->value_);
			//delete theFirst;      // and the old dummy // This is line: 'a'

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