#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <cassert> //for assert()
#include <cmath>
using namespace std;

//#include "Multithreading\Multithreading_SingleProducerMultipleConsumers_v1.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	template <typename T>
	class FixedSizeUnsafeQueue_v1
	{
	public:
		FixedSizeUnsafeQueue_v1(size_t maxSize)
			: maxSize_{ maxSize },
			head_{ 0 },
			size_{ 0 },
			ringBuffer_{ maxSize }
		{
		}

		bool push(T&& obj)
		{
			if (size() == maxSize_)
				return false;

			ringBuffer_[head_] = std::move(obj);
			++head_;
			++size_;
			head_ %= maxSize_;

			return true;
		}

		//exception SAFE pop() version
		bool pop(T& outVal)
		{
			if (size() == 0)
				return false;

			//case 1: maxSize_ = 10, head_ = 7, size_ = 3, then tail = 4
			//x, x, x, x, 4, 5, 6, x, x, x
			//case 2: maxSize_ = 10, head_ = 4, size_ = 6, then tail = 8
			//0, 1, 2, 3, x, x, x, x, 8, 9
			size_t tail = head_ > size_ ? head_ - size_ : maxSize_ - (size_ - head_);
			outVal = std::move(ringBuffer_[tail]);
			--size_;

			return true;
		}

		size_t size()
		{
			return size_;
		}

		bool empty()
		{
			return size_ == 0;
		}

	private:
		size_t maxSize_;
		size_t head_;
		size_t size_;
		std::vector<T> ringBuffer_;
	};
}