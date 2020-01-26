#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <cassert> //for assert()
#include <cmath>
#include <limits>
using namespace std;

//#include "Multithreading\Multithreading_SingleProducerMultipleConsumers_v1.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

#ifdef __GNUC__
#define atomic_inc(ptr) __sync_fetch_and_add ((ptr), 1)
#define compiler_level_memory_fence (asm volatile("" ::: "memory"))
#elif defined (_WIN32)
#include <Windows.h>
#undef max
#undef min
#define atomic_inc(ptr) InterlockedIncrement ((ptr))
#define __builtin_expect(condition, x) (condition)
#define compiler_level_memory_fence (_ReadWriteBarrier())
#else
#error "Need some more porting work here"
#endif

#if !defined(DCACHE1_LINESIZE) || !DCACHE1_LINESIZE
#ifdef DCACHE1_LINESIZE
#undef DCACHE1_LINESIZE
#endif
#define DCACHE1_LINESIZE 64
#endif
#ifdef WIN32
#define ____cacheline_aligned __declspec(align(DCACHE1_LINESIZE))
#else
#define ____cacheline_aligned	__attribute__((aligned(DCACHE1_LINESIZE)))
#endif

/*
Note: This queue does NOT work.
This is Multi Producers Multi Consumers Fixed Size Lock Free Queue.
This is implemented using native (compiler specific) atomic operations.
Reference: http://natsys-lab.blogspot.com/2013/05/lock-free-multi-producer-multi-consumer.html
https://github.com/tempesta-tech/blog/blob/master/lockfree_rb_q.cc
*/

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx(size_t maxSize, size_t numProducers, size_t numConsumers)
			: maxSize_(maxSize), 
			vec_(maxSize),
			size_(0),
			numProducers_(numProducers),
			numConsumers_(numConsumers),
			head_(0),
			tail_(0),
			lastHead_(0),
			lastTail_(0),
			threadPos_((numProducers > numConsumers ? numProducers : numConsumers), ThreadPosition{ std::numeric_limits<unsigned long>::max(), std::numeric_limits<unsigned long>::max() })
		{
		}

		void push(T&& obj, int threadId)
		{
			ThreadPosition& tp = threadPos_[threadId];

			tp.head = head_;
			tp.head = atomic_inc(&head_);

			while (__builtin_expect(tp.head >= lastTail_ + maxSize_, 0))
			{
				auto min = tail_;

				// Update the last_tail_.
				for (size_t i = 0; i < numConsumers_; ++i) {
					auto tmp_t = threadPos_[i].tail;

					// Force compiler to use tmp_h exactly once.
					compiler_level_memory_fence;

					if (tmp_t < min)
						min = tmp_t;
				}
				lastTail_ = min;

				if (tp.head < lastTail_ + maxSize_)
					break;
				_mm_pause();
			}

			size_t pos = tp.head % maxSize_;
			vec_[pos] = obj;
			++size_;

			cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " at index " << pos << " into queue. Queue size: " << size_;

			// Allow consumers eat the item.
			tp.head = ULONG_MAX;
		}

		//exception UNSAFE pop() version
		T pop(int threadId)
		{
			ThreadPosition& tp = threadPos_[threadId];

			tp.tail = tail_;
			tp.tail = atomic_inc(&tail_);

			while (__builtin_expect(tp.tail >= lastHead_, 0))
			{
				auto min = head_;

				// Update the last_head_.
				for (size_t i = 0; i < numProducers_; ++i) {
					auto tmp_h = threadPos_[i].head;

					// Force compiler to use tmp_h exactly once.
					compiler_level_memory_fence;

					if (tmp_h < min)
						min = tmp_h;
				}
				lastHead_ = min;

				if (tp.tail < lastHead_)
					break;
				_mm_pause();
			}

			size_t pos = tp.tail % maxSize_;
			T obj = vec_[pos];
			--size_;
			// Allow producers rewrite the slot.
			tp.tail = std::numeric_limits<unsigned long>::max();
			cout << "\nThread " << this_thread::get_id() << " popped " << obj << " at index " << pos << " from queue. Queue size: " << size_;
			return obj;
		}

		size_t size()
		{
			return size_;
		}

		bool empty()
		{
			return vec_.empty();
		}

	private:
		struct ThreadPosition {
			size_t head, tail;
		};

		size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		size_t size_;
		size_t numProducers_;
		size_t numConsumers_;
#ifdef WIN32
		____cacheline_aligned size_t head_; //stores the index where next element will be pushed
		____cacheline_aligned size_t tail_; //stores the index of object which will be popped
#else
		size_t head_ ____cacheline_aligned; //stores the index where next element will be pushed
		size_t tail_ ____cacheline_aligned; //stores the index of object which will be popped
#endif
		size_t lastHead_;
		size_t lastTail_;
		std::vector<ThreadPosition> threadPos_;
	};
	
}