//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include <thread>
#include <random>
#include <queue>
#include <atomic>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"
#include "ReadWriteLock_std_v1.h"

namespace mm {

	namespace readWriteLockTesting {

		class Object
		{
		public:
			Object(int size)
				: data_{ size }
			{
				//data_.reserve(size);
				//std::random_device rd;
				//std::mt19937 mt(rd());
				//std::uniform_int_distribution<int> dist(1, 100000);
				//for (int i = 0; i < size; ++i)
					//data_.push_back(dist(mt));
					//data_.push_back(i);
			}
			Object(const Object&) = default;
			Object(Object&&) = default;
			Object& operator=(const Object&) = default;
			Object& operator=(Object&&) = default;

			int getSum()
			{
				//int sum = 0;
				//for (int i = 0; i < data_.size(); ++i)
				//	sum += data_[i];
				//return sum;

				return data_;
			}

		private:
			//std::vector<int> data_;
			int data_;
		};

		template<typename T, typename Lock>
		class ThreadSafeQueue
		{
		public:
			void push(T&& obj)
			{
				lock_.acquireWriteLock();
				buffer_.push(std::move(obj));
				lock_.releaseWriteLock();
			}

			void pop()
			{
				if (empty())
					throw std::runtime_error{ "Queue underflow" };

				lock_.acquireWriteLock();
				buffer_.pop();
				lock_.releaseWriteLock();
			}

			const T& top()
			{
				if (empty())
					throw std::runtime_error{ "Queue underflow" };

				lock_.acquireReadLock();
				const T& retVal = buffer_.front();
				lock_.releaseReadLock();

				return retVal;
			}

			bool empty()
			{
				lock_.acquireReadLock();
				bool retVal = buffer_.empty();
				lock_.releaseReadLock();

				return retVal;
			}

		private:
			std::queue<T> buffer_;
			Lock lock_;
		};

		enum class Operations
		{
			push,
			pop,
			top
		};

		template<typename T>
		int testReadWriteLock(const std::string& msg, T& tsq, const std::vector<Operations>& ops)
		{
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(1, 100);

			auto threadFunPushPop = [](T& tsq, int iterations) {
				for (int i = 1; i <= iterations; ++i)
				{
					if (i % 3 == 0)
						tsq.pop();
					else
						tsq.push(Object{ i });
				}
			};

			auto threadFunTop = [](T& tsq, int iterations, std::atomic<int>& totalSum) {
				for (int i = 0; i < iterations; ++i)
				{
					//if (tsq.empty())
					//{
					//	++emptyCount;
					//	continue;
					//}

					Object obj = tsq.top();
					int sum = obj.getSum();
					totalSum += sum;
					break;
				}
			};

			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
			//int totalSum = 0;
			//for (int i = 0; i < ops.size(); ++i)
			//{
			//	switch (ops[i])
			//	{
			//	case Operations::push:
			//		tsq.push(Object{ 10 });
			//		break;
			//	case Operations::pop:
			//		tsq.pop();
			//		break;
			//	case Operations::top:
			//	{
			//		Object obj = tsq.top();
			//		int sum = obj.getSum();
			//		totalSum += sum;
			//		break;
			//	}
			//	default:
			//		throw std::runtime_error{ "Unknown operation: " + std::to_string(static_cast<int>(ops[i])) };
			//	}
			//}
			int numWriters = 50;
			std::vector<std::thread> writers;
			writers.reserve(numWriters);
			for (int i = 0; i < numWriters; ++i)
			{
				writers.push_back(std::thread{ threadFunPushPop, std::ref(tsq), 100000 });
			}

			int numReaders = 50;
			std::vector<std::thread> readers;
			readers.reserve(numReaders);
			std::atomic<int> totalSum = 0;
			for (int i = 0; i < numReaders; ++i)
			{
				readers.push_back(std::thread{ threadFunTop, std::ref(tsq), 100000, std::ref(totalSum) });
			}

			for (int i = 0; i < writers.size(); ++i)
			{
				if (writers[i].joinable())
					writers[i].join();
			}
			for (int i = 0; i < readers.size(); ++i)
			{
				if (readers[i].joinable())
					readers[i].join();
			}
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
			
			long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
			std::cout << "\n" << std::setw(20) << msg
				<< " duration: " << std::setw(12) << duration << " ns"
				<< " totalSum: " << std::setw(6) << totalSum
				//<< " emptyCount: " << std::setw(6) << emptyCount
				;

			return totalSum;
		}

		void testAllReadWriteLocks()
		{
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(1, 100);

			std::vector<Operations> ops;
			//int iterations = 10000;
			//int queueSize = 0;
			//for (int i = 0; i < iterations; ++i)
			//{
			//	int randomNum = dist(mt);
			//	if (queueSize != 0)
			//	{
			//		if (randomNum % 10 == 0)
			//		{
			//			ops.push_back(Operations::pop);
			//			--queueSize;
			//		}
			//		else
			//			ops.push_back(Operations::top);
			//	}
			//	else
			//	{
			//		ops.push_back(Operations::push);
			//		++queueSize;
			//	}
			//}

			std::cout.imbue(std::locale{ "" });
			ThreadSafeQueue<Object, readWriteLock_std_v1::stdMutex> tsq1; testReadWriteLock("stdMutex", tsq1, ops);
			ThreadSafeQueue<Object, readWriteLock_std_v1::stdReadWriteLock> tsq2; testReadWriteLock("stdReadWriteLock", tsq2, ops);
		}

	}


	MM_DECLARE_FLAG(ReadWriteLock);

	MM_UNIT_TEST(ReadWriteLock_Test, ReadWriteLock)
	{
		readWriteLockTesting::testAllReadWriteLocks();
	}
}

