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
#include <forward_list>
#include <atomic>
using namespace std;

/*
This is Multi Producers Multi Consumers Unlimited Size Queue.
This is implemented using two mutexes and one condition variable.
It uses the custom implementation of ForwardList
Producers never need to wait because its unlimited queue and will never be full.
Consumers have to wait if the queue is empty.

Its a modified version of Herb Sutter's lock free queue. It uses locks for producer and consumers and also when it needs to wait.
*/

#define CACHE_LINE_SIZE 64

namespace mm {

	template<typename T>
	class MultiProducersMultiConsumersUnlimitedQueue_v3
	{
	private:
		struct Node
		{
			Node(T obj)
				: data_{ std::move(obj) }, next_a{ nullptr }
			{
			}
			T data_;
			atomic<Node*> next_a;
			char pad[CACHE_LINE_SIZE - sizeof(T) - sizeof(atomic<Node*>)];
		};

		class ForwardList
		{
		public:
			ForwardList()
				: head_{ new Node(T{}) },
				tail_{head_}
			{}

			~ForwardList()
			{
				if (head_ == nullptr)
					return;

				Node* curr = head_;
				do
				{
					Node* removed = curr;
					curr = curr->next_a;
					delete removed;
				} while (curr != nullptr);
			}

			void pop_front(T& outVal)
			{
				Node* removed = head_;
				Node* theNext = head_->next_a;
				outVal = theNext->data_;
				head_ = theNext;
				//if (head_ == nullptr)
				//	tail_ = nullptr;
				delete removed;
			}

			void push_back(T obj)
			{
				Node* pn = new Node(std::move(obj));
				//if (tail_ == nullptr)
				//	head_ = pn;
				//else
					tail_->next_a = pn;

				tail_ = pn;
			}

		public:
			Node* head_;
			Node* tail_;
		};

	public:
		MultiProducersMultiConsumersUnlimitedQueue_v3()
			//: nonAtomicSize_{ 0 }
		{}

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			queue_.push_back(std::move(obj)); //Push element at the tail.
			//++nonAtomicSize_;

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
			p_lock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cv_.notify_one(); //This will always notify one thread even though there are no waiting threads
		}

		//exception SAFE pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);

			//Use double check locking pattern (same as the one used for therad safe singleton)
			if (queue_.head_->next_a == nullptr)
			{
				std::unique_lock<std::mutex> p_lock(mutexProducer_);
				//If the thread is active due to spurious wake-up or more number of threads are notified than the number of elements in queue, 
				//force it to check if queue is empty so that it can wait again if the queue is empty
				while (queue_.head_->next_a == nullptr)
				{
					//cv_.wait(p_lock);
					if (cv_.wait_for(p_lock, timeout) == std::cv_status::timeout)
						return false;
				}
			}
			//OR
			//cond_.wait(mlock, [this](){ return !this->queue_.empty(); });
			//cv_.wait_for(mlock, timeout, [this](){ return !this->queue_.empty(); });

			//if (nonAtomicSize_ == 1)
			//{
			//	--nonAtomicSize_;
			//}
			//else
			//{
			//	--nonAtomicSize_;
			//	p_lock.unlock(); //Release the lock as this consumer thread is working on a part of queue which will not be touched by any producer thread because size > 1
			//}

			queue_.pop_front(outVal);
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true;
		}

		size_t size()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			size_t size = 0;
			Node* curr = queue_.head_->next_a;
			for (; curr != nullptr; curr = curr->next_a)      // release the list
			{
				++size;
			}

			//return size > 0 ? size - 1 : 0;
			return size;
		}

		bool empty()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			//return nonAtomicSize_ == 0;
			return queue_.head_->next_a == nullptr;
		}

	private:
		ForwardList queue_;
		//std::atomic<size_t> size_;
		//size_t nonAtomicSize_;
		std::mutex mutexProducer_;
		std::mutex mutexConsumer_;
		std::condition_variable cv_;
	};


}