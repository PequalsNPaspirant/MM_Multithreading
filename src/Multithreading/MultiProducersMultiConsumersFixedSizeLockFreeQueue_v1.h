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
#define __builtin_expect(condition) (condition)
#define compiler_level_memory_fence (_ReadWriteBarrier())
#else
#error "Need some more porting work here"
#endif

namespace mm {

	template <typename T>
	class MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1
	{
	public:
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1(size_t maxSize, size_t numProducers, size_t numConsumers)
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
			ThreadPosition tp = threadPos_[threadId];

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

			vec_[tp.head % maxSize_] = obj;

			cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << size_;

			// Allow consumers eat the item.
			tp.head = ULONG_MAX;
		}

		//exception UNSAFE pop() version
		T pop(int threadId)
		{
			ThreadPosition tp = threadPos_[threadId];

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

			T obj = vec_[tp.tail % maxSize_];
			// Allow producers rewrite the slot.
			tp.tail = ULONG_MAX;
			cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << size_;
			return obj;
		}

		size_t size()
		{
			return size_;
		}

	private:
		struct ThreadPosition {
			unsigned long head, tail;
		};

		size_t maxSize_;
		std::vector<T> vec_; //This will be used as ring buffer / circular queue
		size_t size_;
		size_t numProducers_;
		size_t numConsumers_;
		size_t head_; //stores the index where next element will be pushed
		size_t tail_; //stores the index of object which will be popped
		size_t lastHead_;
		size_t lastTail_;
		std::vector<ThreadPosition> threadPos_;
	};
	
}