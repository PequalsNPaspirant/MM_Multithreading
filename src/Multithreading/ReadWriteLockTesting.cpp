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

#include "ReadWriteLock_NoPref_v1.h"
#include "ReadWriteLock_NoPref_v2.h"
#include "ReadWriteLock_NoPref_v3.h"
#include "ReadWriteLock_NoPref_v4.h"
#include "ReadWriteLock_NoPref_LockFree_v1.h"
#include "ReadWriteLock_NoPref_LockFree_v2.h"
#include "ReadWriteLock_NoPref_LockFree_v3.h"
#include "ReadWriteLock_NoPref_LockFree_v4.h"
#include "ReadWriteLock_NoPref_LockFree_v5.h"
#include "ReadWriteLock_NoPref_LockFree_v6.h"

#include "ReadWriteLock_ReadPref_v1.h"
#include "ReadWriteLock_ReadPref_v2.h"
#include "ReadWriteLock_ReadPref_v3.h"
#include "ReadWriteLock_ReadPref_LockFree_v1.h"
#include "ReadWriteLock_ReadPref_LockFree_v2.h"
#include "ReadWriteLock_ReadPref_LockFree_v3.h"
#include "ReadWriteLock_ReadPref_LockFree_v4.h"

#include "ReadWriteLock_WritePref_v1.h"
#include "ReadWriteLock_WritePref_v2.h"
#include "ReadWriteLock_WritePref_v3.h"

namespace mm {

	namespace readWriteLockTesting {

		class Object
		{
		public:
			Object(size_t size)
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

			size_t getSum()
			{
				//int sum = 0;
				//for (int i = 0; i < data_.size(); ++i)
				//	sum += data_[i];
				//return sum;

				return data_;
			}

		private:
			//std::vector<size_t> data_;
			size_t data_;
		};

		template<typename ObjectType, typename LockType>
		class ThreadSafeQueue
		{
		public:
			void push(ObjectType&& obj)
			{
				lock_.acquireWriteLock();				
				size_.reset();

				buffer_.push(std::move(obj));

				size_ = std::make_unique<size_t>(buffer_.size());
				lock_.releaseWriteLock();
			}

			void pop()
			{
				if (empty())
					throw std::runtime_error{ "Queue underflow" };

				lock_.acquireWriteLock();
				size_.reset();

				buffer_.pop();

				size_ = std::make_unique<size_t>(buffer_.size());
				lock_.releaseWriteLock();
			}

			const ObjectType& front()
			{
				if (empty())
					throw std::runtime_error{ "Queue underflow" };

				lock_.acquireReadLock();
				size_t* sz = size_.get();
				if (sz == nullptr || buffer_.size() != *sz)
					throw std::runtime_error{ "Size is not equal to queue size" };

				const ObjectType& retVal = buffer_.front();

				if (sz == nullptr || buffer_.size() != *sz)
					throw std::runtime_error{ "Size is not equal to queue size" };
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
			std::queue<ObjectType> buffer_;
			std::unique_ptr<size_t> size_; //Intentionally keeping unique_ptr so that write op will change the memory location
			LockType lock_;
		};

		enum class Operations
		{
			push,
			pop,
			top
		};

		template<typename LockType>
		size_t testReadWriteLock(const std::string& msg, const std::vector<Operations>& ops)
		{
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> dist(1, 100);

			auto threadFunPushPop = [](ThreadSafeQueue<Object, LockType>& tsq, int iterations) {
				for (size_t i = 1; i <= iterations; ++i)
				{
					//First strategy - does not work well
					//Object obj = tsq.front();
					//tsq.pop();
					//if(obj.getSum() == 1)
					//	tsq.push(Object{ 2 });
					//else
					//	tsq.push(Object{ 1 });

					//Second strategy - works
					//if (i % 2 == 0)
					//	tsq.pop();
					//else
					//	tsq.push(Object{ i });

					//Third strategy - works
					tsq.push(Object{ i });
					tsq.pop();
				}
			};

			auto threadFunFront = [](ThreadSafeQueue<Object, LockType>& tsq, int iterations, std::atomic<size_t>& totalSum) {
				for (int i = 0; i < iterations; ++i)
				{
					//if (tsq.empty())
					//{
					//	++emptyCount;
					//	continue;
					//}

					Object obj = tsq.front();
					size_t sum = obj.getSum();
					totalSum += sum;
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

			//constexpr const int iterations = 1'000'000;
			constexpr const int iterations = 100'000;
			constexpr const int numWriters = 50;
			ThreadSafeQueue<Object, LockType> tsq;

			std::vector<std::thread> writers;
			writers.reserve(numWriters);
			for (int i = 0; i < numWriters; ++i)
			{
				writers.push_back(std::thread{ threadFunPushPop, std::ref(tsq), iterations });
			}

			constexpr const int numReaders = 50;
			std::vector<std::thread> readers;
			readers.reserve(numReaders);
			std::atomic<size_t> totalSum = 0;
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
			std::cout << "\n" << std::setw(35) << msg
				<< " duration: " << std::setw(18) << duration << " ns"
				<< "   totalSum: " << std::setw(18) << totalSum
				//<< " emptyCount: " << std::setw(6) << emptyCount
				;

			return totalSum;
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
					cvPause_.wait(lock);

				pause_ = true; //reset it to notify thread waiting on cvResume_ and for all next threads to wait on cvPause_
				lock.unlock();

				cvResume_.notify_one();
			}

			void resume()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				while (!pause_)
					cvResume_.wait(lock);

				pause_ = false; //reset it to notify thread waiting on cvPause_ and for all next threads to wait on cvResume_
				lock.unlock();

				cvPause_.notify_one();
			}

			//void resetFlag()
			//{
			//	std::unique_lock<std::mutex> lock{ mu_ };
			//	pause_ = true;
			//}

		private:
			std::mutex mu_;
			bool pause_{ true };
			std::condition_variable cvPause_;
			std::condition_variable cvResume_;
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
				size_t* sz = size_.get();
				if (sz == nullptr || data_.size() != *sz)
					throw std::runtime_error{ "Size is not equal to hash map size" };

				auto it = data_.find(str);
				int retVal = -1;
				if (it != data_.end())
					retVal = it->second;
				
				if (sz == nullptr || data_.size() != *sz)
					throw std::runtime_error{ "Size is not equal to hash map size" };

				//wait on cv
				ti.pause();

				if (sz == nullptr || data_.size() != *sz)
					throw std::runtime_error{ "Size is not equal to hash map size" };

				readWriteLock_.releaseReadLock();

				return retVal;
			}

			void set(const std::string& str, int n, ThreadInfo& ti)
			{
				readWriteLock_.acquireWriteLock();
				size_.reset();
				//data_.clear();

				//wait on cv
				ti.pause();

				data_[str] = n;
				size_ = std::make_unique<size_t>(data_.size());
				readWriteLock_.releaseWriteLock();
			}

		private:
			ReadWriteLockType readWriteLock_;
			std::unordered_map<std::string, int> data_;
			std::unique_ptr<size_t> size_{ std::make_unique<size_t>(data_.size()) }; //Intentionally keeping unique_ptr so that write op will change the memory location
		};

		template<typename ReadWriteLockType>
		class Thread
		{
		public:
			Thread(bool isReader)
			{
				auto threadFun = [this](bool isReader) {
					this->threadInfo_.pause();
					//this->threadInfo_.resetFlag(); //reset flag so that it will pause again in get() and set() below

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

			void resume()
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
			ER, //End   Reading
			SW, //Start Writing
			EW  //End   Writing
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
					readers[op.threadId_].resume();
					break;
				case OP::ER:
					readers[op.threadId_].resume();
					break;
				case OP::SW:
					writers[op.threadId_].resume();
					break;
				case OP::EW:
					writers[op.threadId_].resume();
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
				if (i % 2 == 0)
					std::cout << "\r" << std::setw(36) << msg << ": " << i << " operations done";

				testSingleOperationsSet<ReadWriteLockType>(2, 2, ops[i]);
			}

			std::cout << std::endl;
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
			std::cout << "\n\n----testAllReadWriteLocks (faster the readers, sum will be minimum) ----\n";

			///testReadWriteLock<readWriteLock_stdMutex_v1::ReadWriteLock>("readWriteLock_stdMutex_v1", ops);
			///testReadWriteLock<readWriteLock_stdSharedMutex_v1::ReadWriteLock>("readWriteLock_stdSharedMutex_v1", ops);

			//No peferrence
			///testReadWriteLock<readWriteLock_NoPref_v1::ReadWriteLock>("readWriteLock_NoPref_v1", ops);
			///testReadWriteLock<readWriteLock_NoPref_v2::ReadWriteLock>("readWriteLock_NoPref_v2", ops);
			testReadWriteLock<readWriteLock_NoPref_v3::ReadWriteLock>("readWriteLock_NoPref_v3", ops);
			testReadWriteLock<readWriteLock_NoPref_v4::ReadWriteLock>("readWriteLock_NoPref_v4", ops);

			///testReadWriteLock<readWriteLock_NoPref_LockFree_v1::ReadWriteLock>("readWriteLock_NoPref_LockFree_v1", ops);
			///testReadWriteLock<readWriteLock_NoPref_LockFree_v2::ReadWriteLock>("readWriteLock_NoPref_LockFree_v2", ops);
			///testReadWriteLock<readWriteLock_NoPref_LockFree_v3::ReadWriteLock>("readWriteLock_NoPref_LockFree_v3", ops);
			// /*FIXME to make NoPref instead of readPref*/ testReadWriteLock<readWriteLock_NoPref_LockFree_v4::ReadWriteLock>("readWriteLock_NoPref_LockFree_v4", ops);
			// /*FIXME to make NoPref instead of readPref*/ testReadWriteLock<readWriteLock_NoPref_LockFree_v5::ReadWriteLock>("readWriteLock_NoPref_LockFree_v5", ops);
			// /*FIXME to make NoPref instead of readPref*/ testReadWriteLock<readWriteLock_NoPref_LockFree_v6::ReadWriteLock>("readWriteLock_NoPref_LockFree_v6", ops);

			//Read Preferrence
			///testReadWriteLock<readWriteLock_ReadPref_v1::ReadWriteLock>("readWriteLock_ReadPref_v1", ops);
			///testReadWriteLock<readWriteLock_ReadPref_v2::ReadWriteLock>("readWriteLock_ReadPref_v2", ops);
			testReadWriteLock<readWriteLock_ReadPref_v3::ReadWriteLock>("readWriteLock_ReadPref_v3", ops);

			///testReadWriteLock<readWriteLock_ReadPref_LockFree_v1::ReadWriteLock>("ReadWriteLock_ReadPref_LockFree_v1", ops);
			///testReadWriteLock<readWriteLock_ReadPref_LockFree_v2::ReadWriteLock>("ReadWriteLock_ReadPref_LockFree_v2", ops);
			///testReadWriteLock<readWriteLock_ReadPref_LockFree_v3::ReadWriteLock>("ReadWriteLock_ReadPref_LockFree_v3", ops);
			///testReadWriteLock<readWriteLock_ReadPref_LockFree_v4::ReadWriteLock>("ReadWriteLock_ReadPref_LockFree_v4", ops);

			//Write Preferrence
			///testReadWriteLock<readWriteLock_WritePref_v1::ReadWriteLock>("readWriteLock_WritePref_v1", ops);
			///testReadWriteLock<readWriteLock_WritePref_v2::ReadWriteLock>("readWriteLock_WritePref_v2", ops);
			testReadWriteLock<readWriteLock_WritePref_v3::ReadWriteLock>("readWriteLock_WritePref_v3", ops);
		}

		void testAllReadWriteLocksInSteps()
		{
			std::cout << "\n\n----testAllReadWriteLocksInSteps----\n";

			///testAllPermutationsOfOperations<readWriteLock_stdMutex_v1::ReadWriteLock>("readWriteLock_stdMutex_v1");
			///testAllPermutationsOfOperations<readWriteLock_stdSharedMutex_v1::ReadWriteLock>("readWriteLock_stdSharedMutex_v1");
			
			//No peferrence
			///testAllPermutationsOfOperations<readWriteLock_NoPref_v1::ReadWriteLock>("readWriteLock_NoPref_v1");
			///testAllPermutationsOfOperations<readWriteLock_NoPref_v2::ReadWriteLock>("readWriteLock_NoPref_v2");
			testAllPermutationsOfOperations<readWriteLock_NoPref_v3::ReadWriteLock>("readWriteLock_NoPref_v3");
			testAllPermutationsOfOperations<readWriteLock_NoPref_v4::ReadWriteLock>("readWriteLock_NoPref_v4");

			///testAllPermutationsOfOperations<readWriteLock_NoPref_LockFree_v1::ReadWriteLock>("ReadWriteLock_NoPref_LockFree_v1");
			///testAllPermutationsOfOperations<readWriteLock_NoPref_LockFree_v2::ReadWriteLock>("ReadWriteLock_NoPref_LockFree_v2");
			//testAllPermutationsOfOperations<readWriteLock_NoPref_LockFree_v3::ReadWriteLock>("ReadWriteLock_NoPref_LockFree_v3");
			// /*FIXME to make NoPref instead of readPref*/ testAllPermutationsOfOperations<readWriteLock_NoPref_LockFree_v4::ReadWriteLock>("ReadWriteLock_NoPref_LockFree_v4");
			// /*FIXME to make NoPref instead of readPref*/ testAllPermutationsOfOperations<readWriteLock_NoPref_LockFree_v5::ReadWriteLock>("ReadWriteLock_NoPref_LockFree_v5");
			// /*FIXME to make NoPref instead of readPref*/ testAllPermutationsOfOperations<readWriteLock_NoPref_LockFree_v6::ReadWriteLock>("ReadWriteLock_NoPref_LockFree_v6");
			
			//Read Preferrence
			///testAllPermutationsOfOperations<readWriteLock_ReadPref_v1::ReadWriteLock>("readWriteLock_ReadPref_v1");
			///testAllPermutationsOfOperations<readWriteLock_ReadPref_v2::ReadWriteLock>("readWriteLock_ReadPref_v2");
			testAllPermutationsOfOperations<readWriteLock_ReadPref_v3::ReadWriteLock>("readWriteLock_ReadPref_v3");

			///testAllPermutationsOfOperations<readWriteLock_ReadPref_LockFree_v1::ReadWriteLock>("readWriteLock_ReadPref_LockFree_v1");
			///testAllPermutationsOfOperations<readWriteLock_ReadPref_LockFree_v2::ReadWriteLock>("readWriteLock_ReadPref_LockFree_v2");
			///testAllPermutationsOfOperations<readWriteLock_ReadPref_LockFree_v3::ReadWriteLock>("readWriteLock_ReadPref_LockFree_v3");
			///testAllPermutationsOfOperations<readWriteLock_ReadPref_LockFree_v4::ReadWriteLock>("readWriteLock_ReadPref_LockFree_v4");

			//Write Preferrence
			///testAllPermutationsOfOperations<readWriteLock_WritePref_v1::ReadWriteLock>("readWriteLock_WritePref_v1");
			///testAllPermutationsOfOperations<readWriteLock_WritePref_v2::ReadWriteLock>("readWriteLock_WritePref_v2");
			testAllPermutationsOfOperations<readWriteLock_WritePref_v3::ReadWriteLock>("readWriteLock_WritePref_v3");
		}

	}

	MM_DECLARE_FLAG(ReadWriteLock);

	MM_UNIT_TEST(ReadWriteLock_Test, ReadWriteLock)
	{
		std::cout.imbue(std::locale{ "" });

		int runIndex = 0;
		while (true)
		{
			std::cout << "\n\n======================== Run Index " << ++runIndex << "========================\n";
			readWriteLockTesting::testAllReadWriteLocks();
			readWriteLockTesting::testAllReadWriteLocksInSteps();
		}
	}
}

