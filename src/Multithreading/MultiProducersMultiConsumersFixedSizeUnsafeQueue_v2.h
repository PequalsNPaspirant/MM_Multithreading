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
	class FixedSizeUnsafeQueue_v2
	{
	public:
		FixedSizeUnsafeQueue_v2(size_t maxSize)
			: maxSize_{ maxSize },
			head_{ 0 },
			tail_{ 0 },
			ringBuffer_{ getNextPowerOfTwo(maxSize) }
		{
		}

		bool push(T&& obj)
		{
			if (size() == maxSize_)
				return false;

			ringBuffer_[head_] = std::move(obj);
			++head_;
			//head_ %= maxSize_;

			return true;
		}

		//exception SAFE pop() version
		bool pop(T& outVal)
		{
			if (size() == 0)
				return false;

			outVal = std::move(ringBuffer_[tail_]);
			++tail_;
			//tail_ %= maxSize_;

			return true;
		}

		size_t size()
		{
			return head_ - tail_;
		}

		bool empty()
		{
			return head_ == tail_;
		}

	private:
		size_t maxSize_;
		size_t head_;
		size_t tail_;
		std::vector<T> ringBuffer_;

		size_t getNextPowerOfTwo(size_t num)
		{
			return num;
		}
	};
}