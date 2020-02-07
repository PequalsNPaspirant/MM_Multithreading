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
using namespace std;

/*
This is Multi Producers Multi Consumers Unlimited Size Queue.
This is implemented using two mutexes and one condition variable.
It uses std::queue<T, Container<T>> to store data where Container is std::list and std::forward_list
Producers never need to wait because its unlimited queue and will never be full.
Consumers have to wait if the queue is empty.
*/

namespace mm {

	//template <typename T, typename Container>
	template<typename T, template <typename... Args> class Container>
	class MultiProducersMultiConsumersUnlimitedQueue_v2
	{
		//Its empty class template. It supports only below three types.
	};

	template<typename T>
	class MultiProducersMultiConsumersUnlimitedQueue_v2<T, std::list>
	{
	public:
		MultiProducersMultiConsumersUnlimitedQueue_v2()
			: 
			//head_{ queue_.begin() },
			//tail_{ queue_.end() },
			nonAtomicSize_{	0 }
		{}

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			queue_.push_back(std::move(obj));
			++nonAtomicSize_;
			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
			p_lock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cv_.notify_one(); //This will always notify one thread even though there are no waiting threads
		}

		//exception SAFE pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);

			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			//If the thread is active due to spurious wake-up or more number of threads are notified than the number of elements in queue, 
			//force it to check if queue is empty so that it can wait again if the queue is empty
			while(nonAtomicSize_ == 0) //Do not use 'while(queue_.empty())' because it internally uses size_ inside std::list which is not protected/thread safe
			{
				//cv_.wait(mlock);
				if(cv_.wait_for(p_lock, timeout) == std::cv_status::timeout)
					return false;
			}
			//OR
			//cv_.wait(mlock, [this](){ return !this->queue_.empty(); });
			//cv_.wait_for(mlock, timeout, [this](){ return !this->queue_.empty(); });

			//if (queue_.size() > 1) //Do not use this size() because it may be updated concurrently. We are not protecting write access to it by multiple threads.
			if(nonAtomicSize_ > 1)
			{
				--nonAtomicSize_; //protect this under mutex for producer
				p_lock.unlock(); //Release the lock as this consumer thread is working on a part of queue which will not be touched by any producer thread because size > 1
			}
			else
				--nonAtomicSize_;

			outVal = queue_.front();
			queue_.pop_front();
			
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true;
		}

		size_t size()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			//return queue_.size();
			return nonAtomicSize_;
		}

		bool empty()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			//return queue_.empty();
			return nonAtomicSize_ == 0;
		}

	private:
		std::list<T> queue_;
		//typename std::list<T>::iterator head_;
		//typename std::list<T>::iterator tail_;
		//std::atomic<size_t> size_;
		size_t nonAtomicSize_;
		std::mutex mutexProducer_;
		std::mutex mutexConsumer_;
		std::condition_variable cv_;
	};



	template<typename T>
	class MultiProducersMultiConsumersUnlimitedQueue_v2<T, std::forward_list>
	{
	public:
		MultiProducersMultiConsumersUnlimitedQueue_v2()
			: last_{ queue_.before_begin() },
			nonAtomicSize_{ 0 }
		{}

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			if(nonAtomicSize_ == 0) //if(queue_.empty()) also works but better check the value of nonAtomicSize_
				last_ = queue_.before_begin();
			last_ = queue_.insert_after(last_, std::move(obj)); //Push element at the tail.
			++nonAtomicSize_;
			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
			p_lock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cv_.notify_one(); //This will always notify one thread even though there are no waiting threads
		}

		//exception SAFE pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);

			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			//If the thread is active due to spurious wake-up or more number of threads are notified than the number of elements in queue, 
			//force it to check if queue is empty so that it can wait again if the queue is empty
			while(nonAtomicSize_ == 0) //while(queue_.empty()) also works but better check the value of nonAtomicSize_
			{
				//cond_.wait(mlock);
				if (cv_.wait_for(p_lock, timeout) == std::cv_status::timeout)
					return false;
			}
			//OR
			//cv_.wait(p_lock, [this](){ return !this->queue_.empty(); });
			//cv_.wait_for(p_lock, timeout, [this](){ return !this->queue_.empty(); });

			if (nonAtomicSize_ > 1)
			{
				--nonAtomicSize_; //protect this under mutex for producer
				p_lock.unlock(); //Release the lock as this consumer thread is working on a part of queue which will not be touched by any producer thread because size > 1
			}
			else
				--nonAtomicSize_;

			outVal = queue_.front();
			queue_.erase_after(queue_.before_begin());
			//if (queue_.empty())
			//	last_ = queue_.before_begin();

			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true;
		}

		size_t size()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			//return std::distance(queue_.begin(), queue_.end());
			return nonAtomicSize_;
		}

		bool empty()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			//return queue_.empty();
			return nonAtomicSize_ == 0;
		}

	private:
		std::forward_list<T> queue_; //The queue internally uses the vector by default
		typename std::forward_list<T>::iterator last_;
		//std::atomic<size_t> size_;
		size_t nonAtomicSize_;
		std::mutex mutexProducer_;
		std::mutex mutexConsumer_;
		std::condition_variable cv_;
	};




	template <typename... Args> class Undefined {};

	template<typename T>
	class MultiProducersMultiConsumersUnlimitedQueue_v2<T, Undefined>
	{
	private:
		struct Node
		{
			Node(T obj)
				: data_{ std::move(obj) }, next_{ nullptr }
			{
			}
			T data_;
			Node* next_;
		};

		class ForwardList
		{
		public:
			ForwardList()
				: head_{nullptr},
				tail_{nullptr}
			{}

			~ForwardList()
			{
				if (head_ == nullptr)
					return;

				Node* curr = head_;
				do
				{
					Node* removed = curr;
					curr = curr->next_;
					delete removed;
				} while (curr != tail_);
			}

			void pop_front(T& outVal)
			{
				outVal = head_->data_;
				Node* removed = head_;
				head_ = head_->next_;
				if (head_ == nullptr)
					tail_ = nullptr;
				delete removed;
			}

			void push_back(T obj)
			{
				Node* pn = new Node(std::move(obj));
				if (tail_ == nullptr)
					head_ = pn;
				else
					tail_->next_ = pn;

				tail_ = pn;
			}

		private:
			Node* head_;
			Node* tail_;
		};

	public:
		MultiProducersMultiConsumersUnlimitedQueue_v2()
			: nonAtomicSize_{ 0 }
		{}

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			queue_.push_back(std::move(obj)); //Push element at the tail.
			++nonAtomicSize_;

			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
			p_lock.unlock(); //release the lock on mutex, so that the notified thread can acquire that mutex immediately when awakened,
							//Otherwise waiting thread may try to acquire mutex before this thread releases it.
			cv_.notify_one(); //This will always notify one thread even though there are no waiting threads
		}

		//exception SAFE pop() with timeout. Returns false if timeout occurs.
		bool pop(T& outVal, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);

			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			//If the thread is active due to spurious wake-up or more number of threads are notified than the number of elements in queue, 
			//force it to check if queue is empty so that it can wait again if the queue is empty
			while (nonAtomicSize_ == 0)
			{
				//cv_.wait(p_lock);
				if (cv_.wait_for(p_lock, timeout) == std::cv_status::timeout)
					return false;
			}
			//OR
			//cond_.wait(mlock, [this](){ return !this->queue_.empty(); });
			//cv_.wait_for(mlock, timeout, [this](){ return !this->queue_.empty(); });

			if (nonAtomicSize_ == 1)
			{
				--nonAtomicSize_;
			}
			else
			{
				--nonAtomicSize_;
				p_lock.unlock(); //Release the lock as this consumer thread is working on a part of queue which will not be touched by any producer thread because size > 1
			}

			queue_.pop_front(outVal);
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return true;
		}

		size_t size()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			return nonAtomicSize_;
		}

		bool empty()
		{
			std::unique_lock<std::mutex> p_lock(mutexProducer_);
			std::unique_lock<std::mutex> c_lock(mutexConsumer_);
			return nonAtomicSize_ == 0;
		}

	private:
		ForwardList queue_;
		//std::atomic<size_t> size_;
		size_t nonAtomicSize_;
		std::mutex mutexProducer_;
		std::mutex mutexConsumer_;
		std::condition_variable cv_;
	};



	//TODO:
	//Implement the thread safe forward list using std::unique_ptr



	//TODO:
	//Implement the thread safe forward list using std::shared_ptr



	//TODO:
	//Implement the thread safe forward list based on article: https://www.justsoftwaresolutions.co.uk/threading/why-do-we-need-atomic_shared_ptr.html


}