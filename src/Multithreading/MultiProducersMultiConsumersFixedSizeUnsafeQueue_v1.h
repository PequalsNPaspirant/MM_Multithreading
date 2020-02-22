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
			tail_{ 0 },
			size_{ 0 },
			//ringBuffer_{ 0 }
			ringBuffer_{ maxSize }
		{
			//ringBuffer_.reserve(maxSize_);
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

			outVal = std::move(ringBuffer_[tail_]);
			++tail_;
			--size_;
			tail_ %= maxSize_;

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
		size_t tail_;
		size_t size_;
		std::vector<T> ringBuffer_;
	};
}