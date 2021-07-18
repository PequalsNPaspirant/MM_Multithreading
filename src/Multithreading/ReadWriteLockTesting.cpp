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
#include <utility>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"
#include "ReadWriteLock_std_v1.h"
#include "ReadWriteLock_ReadPref_v1.h"
#include "ReadWriteLock_WritePref_v1.h"
#include "ReadWriteLock_NoPref_v1.h"
#include "ReadWriteLock_NoPref_v2.h"

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

			const T& front()
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
		int testReadWriteLock(const std::string& msg, const std::vector<Operations>& ops)
		{
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(1, 100);

			auto threadFunPushPop = [](ThreadSafeQueue<Object, T>& tsq, int iterations) {
				for (int i = 1; i <= iterations; ++i)
				{
					if (i % 3 == 0)
						tsq.pop();
					else
						tsq.push(Object{ i });
				}
			};

			auto threadFunFront = [](ThreadSafeQueue<Object, T>& tsq, int iterations, std::atomic<int>& totalSum) {
				for (int i = 0; i < iterations; ++i)
				{
					//if (tsq.empty())
					//{
					//	++emptyCount;
					//	continue;
					//}

					Object obj = tsq.front();
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

			int iterations = 1000000;
			int numWriters = 50;
			ThreadSafeQueue<Object, T> tsq;

			std::vector<std::thread> writers;
			writers.reserve(numWriters);
			for (int i = 0; i < numWriters; ++i)
			{
				writers.push_back(std::thread{ threadFunPushPop, std::ref(tsq), iterations });
			}

			int numReaders = 50;
			std::vector<std::thread> readers;
			readers.reserve(numReaders);
			std::atomic<int> totalSum = 0;
			tsq.push(Object{ 0 }); //Push one object to be on safer side in case writers lag behind and reader threads start executing first
			for (int i = 0; i < numReaders; ++i)
			{
				readers.push_back(std::thread{ threadFunFront, std::ref(tsq), iterations, std::ref(totalSum) });
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
			std::cout << "\n\n----testAllReadWriteLocks----\n";

			testReadWriteLock<readWriteLock_stdMutex_v1::ReadWriteLock>("stdMutex", ops);
			testReadWriteLock<readWriteLock_stdSharedMutex_v1::ReadWriteLock>("stdReadWriteLock", ops);
			testReadWriteLock<readWriteLock_NoPref_v1::ReadWriteLock>("stdReadWriteLock", ops);
			testReadWriteLock<readWriteLock_NoPref_v2::ReadWriteLock>("stdReadWriteLock", ops);
		}







		class ThreadInfo
		{
		public:
			ThreadInfo() = default;
			~ThreadInfo() = default;
			ThreadInfo(const ThreadInfo&) = delete;
			//ThreadInfo(ThreadInfo&& rhs) = default;
			ThreadInfo(ThreadInfo&& rhs) :
				//mu_{ std::move(rhs.mu_) },
				pause_{ std::move(rhs.pause_) }
				//cv_{ std::move(rhs.cv_) }
			{
			}
			ThreadInfo& operator=(const ThreadInfo&) = delete;
			ThreadInfo& operator=(ThreadInfo&&) = delete;

			void pause()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				while (pause_)
					cv_.wait(lock);
			}

			void resume()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				pause_ = false;
				lock.unlock();

				cv_.notify_all();
			}

			void resetFlag()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				pause_ = true;
			}

		private:
			std::mutex mu_;
			bool pause_{ true };
			std::condition_variable cv_;
		};


		template<typename ReadWriteLockType>
		class ThreadsafeHashMap
		{
		public:
			ThreadsafeHashMap() = default;
			~ThreadsafeHashMap() = default;
			ThreadsafeHashMap(const ThreadsafeHashMap&) = delete;
			ThreadsafeHashMap(ThreadsafeHashMap&& rhs) :
				//readWriteLock_{ std::move(rhs.readWriteLock_) },
				data_{ std::move(rhs.data_) }
			{
			}
			ThreadsafeHashMap& operator=(const ThreadsafeHashMap&) = delete;
			ThreadsafeHashMap& operator=(ThreadsafeHashMap&&) = delete;

			int get(const std::string& str, ThreadInfo& ti)
			{
				readWriteLock_.acquireReadLock();
				
				auto it = data_.find(str);
				int retVal = -1;
				if (it != data_.end())
					retVal = it->second;
				
				//wait on cv
				ti.pause();

				readWriteLock_.releaseReadLock();

				return retVal;
			}

			void set(const std::string& str, int n, ThreadInfo& ti)
			{
				readWriteLock_.acquireWriteLock();
				data_[str] = n;

				//wait on cv
				ti.pause();

				readWriteLock_.releaseWriteLock();
			}

		private:
			ReadWriteLockType readWriteLock_;
			std::unordered_map<std::string, int> data_;
		};

		template<typename ReadWriteLockType>
		class Thread
		{
		public:
			Thread(bool isReader)
			{
				auto threadFun = [this](bool isReader) {
					this->threadInfo_.pause();
					this->threadInfo_.resetFlag(); //reset flag so that it will pause again in get() and set() below

					if (isReader)
					{
						this->map_.get("ten", threadInfo_);
					}
					else
					{
						this->map_.set("ten", 10, threadInfo_);
					}
				};

				thread_ = std::thread{ threadFun, isReader };
			}
			Thread(const Thread&) = delete;
			Thread(Thread&& rhs) :
				threadInfo_{ std::move(rhs.threadInfo_) },
				thread_{ std::move(rhs.thread_) },
				map_{ std::move(rhs.map_) }
			{
			}
			Thread& operator=(const Thread&) = delete;
			Thread& operator=(Thread&&) = delete;

			~Thread()
			{
				if (thread_.joinable())
					thread_.join();
			}

			void start()
			{
				threadInfo_.resume();
			}

			void end()
			{
				threadInfo_.resume();
			}

		private:
			ThreadInfo threadInfo_;
			std::thread thread_;
			ThreadsafeHashMap<ReadWriteLockType> map_;
		};

		enum class OP
		{
			SR, //Start Reading
			ER, //Env   Reading
			SW, //Start Writing
			EW  //Env   Writing
		};

		struct Operation
		{
			Operation(int threadId, OP op) :
				threadId_{ threadId }, op_{ op }
			{}

			Operation(const Operation&) = default;
			Operation(Operation&&) = default;
			Operation& operator=(const Operation&) = default;
			Operation& operator=(Operation&&) = default;

			bool operator==(const Operation& rhs) const
			{
				return op_ == rhs.op_;
			}

			std::string toString()
			{
				std::string retVal;

				retVal += "{";
				retVal += std::to_string(threadId_);
				retVal += ", ";

				switch (op_)
				{
				case OP::SR:
					retVal += "SR";
					break;
				case OP::ER:
					retVal += "ER";
					break;
				case OP::SW:
					retVal += "SW";
					break;
				case OP::EW:
					retVal += "EW";
					break;
				}
				retVal += "}";

				return retVal;
			}

			int threadId_;
			OP op_;
		};

		template<typename ReadWriteLockType>
		void testSingleOperationsSet(size_t numReaders, size_t numWriters, const std::vector<Operation>& ops)
		{
			ThreadsafeHashMap<ReadWriteLockType> map;
			std::vector< Thread<ReadWriteLockType> > readers;
			readers.reserve(numReaders);
			for(int i = 0; i < numReaders; ++i)
				readers.emplace_back(true);
			std::vector< Thread<ReadWriteLockType> > writers;
			writers.reserve(numWriters);
			for (int i = 0; i < numWriters; ++i)
				writers.emplace_back(false);

			for (const Operation& op : ops)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(1));

				switch (op.op_)
				{
				case OP::SR:
					readers[op.threadId_].start();
					break;
				case OP::ER:
					readers[op.threadId_].end();
					break;
				case OP::SW:
					writers[op.threadId_].start();
					break;
				case OP::EW:
					writers[op.threadId_].end();
					break;
				}
			}
		}

		//Generates all n! permutations
		void generateAllPermutationsUniqueItems(std::vector<Operation>&& op, const int indexOfElementToSwap, std::vector< std::vector<Operation> >& retVal)
		{
			if (op.size() == indexOfElementToSwap)
			{
				retVal.push_back(std::move(op));
				return;
			}

			for (int i = indexOfElementToSwap; i < op.size(); ++i)
			{
				std::vector<Operation> copy = op;
				if (indexOfElementToSwap != i) 
					std::swap(copy[indexOfElementToSwap], copy[i]);

				generateAllPermutationsUniqueItems(std::move(copy), indexOfElementToSwap + 1, retVal);
			}
		}

		//Generates all n! permutations
		void generateAllPermutationsRepeatedItems(std::vector<Operation>&& op, const int indexOfElementToSwap, std::vector< std::vector<Operation> >& retVal)
		{
			if (op.size() == indexOfElementToSwap)
			{
				retVal.push_back(std::move(op));
				return;
			}

			for (int i = indexOfElementToSwap; i < op.size(); ++i)
			{
				if (indexOfElementToSwap != i && op[indexOfElementToSwap] == op[i])
					continue;

				std::vector<Operation> copy = op;
				if (indexOfElementToSwap != i)
					std::swap(copy[indexOfElementToSwap], copy[i]);

				generateAllPermutationsRepeatedItems(std::move(copy), indexOfElementToSwap + 1, retVal);
			}
		}

		//Generates all valid permutations = (n/2)! / ((n/4)!*(n/4)!)  *  ((n/4)/(n/4))! / ((n/4)!*(n/4)!)
		//n/2 read threads and n/2 write threads
		//n/2 start operations (n/4 start read thread op and n/4 start write thread op) can be arranged in (n/2)! / ((n/4)!*(n/4)!) since there are n/4 similar items
		//n/2 end operations (n/4 end read thread op and n/4 end write thread op)can be arranged in n/2 possible places after their respective start operations
		//for n = 8, 4! / (2!*2!) * 4! / (2!*2!) = 3*2 * 3*2 = 36
		void generateAllValidPermutations(const size_t expectedSize, std::vector<Operation>&& op, const int indexOfElementToSwap, std::vector< std::vector<Operation> >& retVal)
		{
			if (op.size() == expectedSize && op.size() == indexOfElementToSwap)
			{
				retVal.push_back(std::move(op));
				return;
			}

			if (indexOfElementToSwap > 0)
			{
				switch (op[indexOfElementToSwap - 1].op_)
				{
				case OP::SR: op.push_back(Operation{ op[indexOfElementToSwap - 1].threadId_, OP::ER });
					break;
				case OP::SW: op.push_back(Operation{ op[indexOfElementToSwap - 1].threadId_, OP::EW });
					break;
				default:
					break;
				}
			}

			for (int i = indexOfElementToSwap; i < op.size(); ++i)
			{
				if (indexOfElementToSwap != i && op[indexOfElementToSwap].op_ == op[i].op_)
					continue;

				std::vector<Operation> copy = op;
				if (indexOfElementToSwap != i)
					std::swap(copy[indexOfElementToSwap], copy[i]);

				generateAllValidPermutations(expectedSize, std::move(copy), indexOfElementToSwap + 1, retVal);
			}
		}

		size_t getFactorial(size_t n)
		{
			size_t retVal = 1;
			for (size_t i = n; i > 0; --i)
				retVal *= i;

			return retVal;
		}

		template<typename ReadWriteLockType>
		void testAllPermutationsOfOperations(const std::string& msg)
		{
			OP SR = OP::SR; //Start Reading
			OP ER = OP::ER; //Env   Reading
			OP SW = OP::SW; //Start Writing
			OP EW = OP::EW; //Env   Writing

			//std::vector<Operation> op{ { {0, SR}, { 0, ER } } };
			std::vector<Operation> op{ { {0, SR}, { 1, SR }, { 0, SW }, { 1, SW }  } };
			//std::vector<Operation> op{ { {0, SR}, { 0, ER }, { 1, SR }, { 1, ER }, { 0, SW }, { 0, EW }, { 1, SW }, { 1, EW }  } };
			std::vector< std::vector<Operation> > ops;
			size_t size = getFactorial(op.size());
			ops.reserve(size);
			
			generateAllValidPermutations(2 * op.size(), std::move(op), 0, ops);

			//{
			//	{ {0, SR}, {0, ER}, {1, SR}, {1, ER}, {0, SW}, {0, EW}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {1, SR}, {0, ER}, {1, ER}, {0, SW}, {0, EW}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {1, SR}, {1, ER}, {0, ER}, {0, SW}, {0, EW}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {1, SR}, {1, ER}, {0, SW}, {0, ER}, {0, EW}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {1, SR}, {1, ER}, {0, SW}, {0, EW}, {0, ER}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {1, SR}, {1, ER}, {0, SW}, {0, EW}, {1, SW}, {0, ER}, {1, EW}  },
			//	{ {0, SR}, {1, SR}, {1, ER}, {0, SW}, {0, EW}, {1, SW}, {1, EW}, {0, ER}  },
			//	{ {0, SR}, {0, ER}, {1, SR}, {0, SW}, {1, ER}, {0, EW}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {0, ER}, {1, SR}, {0, SW}, {0, EW}, {1, ER}, {1, SW}, {1, EW}  },
			//	{ {0, SR}, {0, ER}, {1, SR}, {0, SW}, {0, EW}, {1, SW}, {1, ER}, {1, EW}  },
			//	{ {0, SR}, {0, ER}, {1, SR}, {0, SW}, {0, EW}, {1, SW}, {1, EW}, {1, ER}  },
			//	{ {0, SR}, {0, ER}, {1, SR}, {1, ER}, {0, SW}, {0, EW}, {1, SW}, {1, EW}  },
			//};

			//for (int i = 0; i < ops.size(); ++i)
			//{
			//	std::cout << "\n";
			//	for (int j = 0; j < ops[i].size(); ++j)
			//	{
			//		std::cout << ops[i][j].toString() << ", ";
			//	}
			//}

			for (int i = 0; i < ops.size(); ++i)
			{
				if (i % 10 == 0)
					std::cout << "\rDone testSingleOperationsSet() for: " << i;

				testSingleOperationsSet<ReadWriteLockType>(2, 2, ops[i]);
			}
		}

		void testAllReadWriteLocksInSteps()
		{
			std::cout << "\n\n----testAllReadWriteLocksInSteps----\n";

			testAllPermutationsOfOperations<readWriteLock_stdMutex_v1::ReadWriteLock>("stdMutex");
			testAllPermutationsOfOperations<readWriteLock_stdSharedMutex_v1::ReadWriteLock>("stdReadWriteLock");
			testAllPermutationsOfOperations<readWriteLock_ReadPref_v1::ReadWriteLock>("stdReadWriteLock");
		}

	}

	MM_DECLARE_FLAG(ReadWriteLock);

	MM_UNIT_TEST(ReadWriteLock_Test, ReadWriteLock)
	{
		std::cout.imbue(std::locale{ "" });
		readWriteLockTesting::testAllReadWriteLocks();
		readWriteLockTesting::testAllReadWriteLocksInSteps();
	}
}

