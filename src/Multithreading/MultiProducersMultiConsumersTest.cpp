#include <iostream>
#include <random>
#include <chrono>
#include <locale>
#include <memory>
#include <vector>
#include <list>
#include <forward_list>
#include <limits>
#include <unordered_map>
#include <type_traits>
using namespace std;

#include "MultiProducersMultiConsumersUnlimitedQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v2.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v3.h"

#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v2.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v5.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v6.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v7.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v8.h"

#include "MultiProducersMultiConsumersFixedSizeQueue_v1.h"
#include "MultiProducersMultiConsumersFixedSizeQueue_v2.h"
#include "MultiProducersMultiConsumersFixedSizeQueue_v3.h"

#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v6.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx.h"

#include "MultiProducersMultiConsumersUnlimitedUnsafeQueue_v1.h"

#include "MM_HighResolutionClock.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"


namespace mm {

	//================ Global Variables to collect the test results ================
	std::random_device rd;
	std::mt19937 mt32(rd()); //for 32 bit system
	std::mt19937_64 mt64(rd()); //for 64 bit system
	std::uniform_int_distribution<size_t> dist(0, 999);
	vector<size_t> sleepTimesNanosVec;
	size_t totalSleepTimeNanos;
	bool useSleep = true;
	/*
	Results in the tabular format like below: (where (a,b,c) = a producers b consumers and c operations by each thread)
	(#producdrs, #consumers, #operations)          Queue1       Queue2
	(1,1,1)
	(1,1,10)
	(2,2,2)
	*/

	enum class QueueType
	{
		MPMC_U_v1_deque,
		MPMC_U_v1_list,
		MPMC_U_v1_fwlist,
		MPMC_U_v2_list,
		MPMC_U_v2_fwlist,
		MPMC_U_v2_myfwlist,
		MPMC_U_v3_myfwlist,

		MPMC_U_LF_v1,
		MPMC_U_LF_v2,
		MPMC_U_LF_v3,
		MPMC_U_LF_v4,
		MPMC_U_LF_v5,
		MPMC_U_LF_v6,
		MPMC_U_LF_v7,
		MPMC_U_LF_v8,

		MPMC_FS_v1,
		MPMC_FS_v2,
		MPMC_FS_v3,

		MPMC_FS_LF_v1,
		MPMC_FS_LF_v2,
		MPMC_FS_LF_v3,
		MPMC_FS_LF_v4,
		MPMC_FS_LF_v5,
		MPMC_FS_LF_v6,
		//MPMC_FS_LF_vx,

		maxQueueTypes
	};

	std::unordered_map<QueueType, string> queueTypeToQueueName{
		{ QueueType::MPMC_U_v1_deque, "MPMC_U_v1_deque"},
		{ QueueType::MPMC_U_v1_list, "MPMC_U_v1_list" },
		{ QueueType::MPMC_U_v1_fwlist, "MPMC_U_v1_fwlist" },
		{ QueueType::MPMC_U_v2_list, "MPMC_U_v2_list" },
		{ QueueType::MPMC_U_v2_fwlist, "MPMC_U_v2_fwlist" },
		{ QueueType::MPMC_U_v2_myfwlist, "MPMC_U_v2_myfwlist" },
		{ QueueType::MPMC_U_v3_myfwlist, "MPMC_U_v3_myfwlist" },

		{ QueueType::MPMC_U_LF_v1, "MPMC_U_LF_v1" },
		{ QueueType::MPMC_U_LF_v2, "MPMC_U_LF_v2" },
		{ QueueType::MPMC_U_LF_v3, "MPMC_U_LF_v3" },
		{ QueueType::MPMC_U_LF_v4, "MPMC_U_LF_v4" },
		{ QueueType::MPMC_U_LF_v5, "MPMC_U_LF_v5" },
		{ QueueType::MPMC_U_LF_v6, "MPMC_U_LF_v6" },
		{ QueueType::MPMC_U_LF_v7, "MPMC_U_LF_v7" },
		{ QueueType::MPMC_U_LF_v8, "MPMC_U_LF_v8" },

		{ QueueType::MPMC_FS_v1, "MPMC_FS_v1" },
		{ QueueType::MPMC_FS_v2, "MPMC_FS_v2" },
		{ QueueType::MPMC_FS_v3, "MPMC_FS_v3" },

		{ QueueType::MPMC_FS_LF_v1, "MPMC_FS_LF_v1" },
		{ QueueType::MPMC_FS_LF_v2, "MPMC_FS_LF_v2" },
		{ QueueType::MPMC_FS_LF_v3, "MPMC_FS_LF_v3" },
		{ QueueType::MPMC_FS_LF_v4, "MPMC_FS_LF_v4" },
		{ QueueType::MPMC_FS_LF_v5, "MPMC_FS_LF_v5" },
		{ QueueType::MPMC_FS_LF_v6, "MPMC_FS_LF_v6" }
		//{ QueueType::MPMC_FS_LF_vx, "MPMC_FS_LF_vx"}
	};

	struct ResultSet
	{
		QueueType queueType;
		size_t time;
		bool queueEmpty;
		size_t sizeAtEnd;
	};

	struct TestCase
	{
		TestCase()
			: result_(static_cast<int>(QueueType::maxQueueTypes))
		{}

		TestCase(int numProducers, int numConsumers, int numOperations, int queueSize)
			: numProducers_{ numProducers },
			numConsumers_{ numConsumers },
			numOperations_{ numOperations },
			queueSize_{ queueSize },
			result_(static_cast<int>(QueueType::maxQueueTypes))
		{}
		int numProducers_;
		int numConsumers_;
		int numOperations_;
		int queueSize_;
		std::vector<ResultSet> result_;
	};

	std::vector<TestCase> results;
	std::vector<std::string> columnNames;

	constexpr const int subCol1 = 5;
	constexpr const int subCol2 = 5;
	constexpr const int subCol3 = 10;
	constexpr const int subCol4 = 5;
	constexpr const int firstColWidth = subCol1 + subCol2 + subCol3 + subCol4;
	constexpr const int colWidth = 20;

	void my_runtime_assert(bool expectedCondition)
	{
		if (!expectedCondition)
		{
			int* ptr = nullptr;
			int n = *ptr;
		}
	}

	//================ end of global variables ================

	class Object
	{
	public:
		Object()
			: pNum_{ nullptr }
		{}
		Object(int num)
			: pNum_{ new int{ num } },
			str_(num, '*')
		{}
		~Object()
		{
			delete pNum_;
		}
		Object(const Object& rhs) = delete;
		Object(Object&& rhs)
			: pNum_{ nullptr }
		{
			acquireOwnershipFrom(std::move(rhs));
		}

		Object& operator=(const Object& rhs) = delete;
		Object& operator=(Object&& rhs)
		{
			acquireOwnershipFrom(std::move(rhs));
			return *this;
		}

		int getValue() const
		{
			return *pNum_;
		}

		const string& getStr() const
		{
			return str_;
		}

	private:
		int* pNum_;
		string str_;

		unsigned int dummy_;

		void acquireOwnershipFrom(Object&& rhs)
		{
			if (this == &rhs)
				return;

			//Do some work so that it takes time to move data members
			for (int i = 0; useSleep && i < 500; ++i) { dummy_ += rand(); }

			std::swap(pNum_, rhs.pNum_);
			//delete pNum_;
			//pNum_ = rhs.pNum_;
			//rhs.pNum_ = nullptr;

			for (int i = 0; useSleep && i < 500; ++i) { dummy_ += rand(); }

			std::swap(str_, rhs.str_);
			//str_ = rhs.str_;
			//rhs.str_.clear();

			for (int i = 0; useSleep && i < 500; ++i) { dummy_ += rand(); }
		}
	};

	template<typename T>
	bool validateResults(const T& obj)
	{
		return true;
	}

	template<>
	bool validateResults<Object>(const Object& obj)
	{
		int n = obj.getValue();
		const string& str = obj.getStr();
		my_runtime_assert(n == str.length());
		return true;
	}

	template<typename Tqueue, typename Tobj>
	void producerThreadFunction(Tqueue& queue, size_t numProdOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numProdOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				//int sleepTime = dist(mt64) % 100;
				size_t sleepTime = sleepTimesNanosVec[threadId * numProdOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}
			//int n = dist(mt64);
			//cout << "\nThread " << this_thread::get_id() << " pushing " << n << " into queue";
			int n = i;
			Tobj obj{ n % 256 + 1 };
			queue.push(std::move(obj));
		}
	}


	template<typename Tqueue, typename Tobj>
	void consumerThreadFunction(Tqueue& queue, size_t numConsOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numConsOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				//int sleepTime = dist(mt64) % 100;
				size_t sleepTime = sleepTimesNanosVec[threadId * numConsOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}

			{
				Tobj obj;
				//long long timeout = std::numeric_limits<long long>::max();
				std::chrono::milliseconds timeoutMilisec{ 1000 * 60 * 60 }; // timeout = 1 hour
				bool result = queue.pop(obj, timeoutMilisec);
				my_runtime_assert(result);

				validateResults<Tobj>(obj);

				if (!result)
				{
					int n = 100; //This is just for debugging purpose
					int * p = nullptr;
					*p = n;
				}
			}
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}

	struct separate_thousands : std::numpunct<char> {
		char_type do_thousands_sep() const override { return ','; }  // separate with commas
		string_type do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	template<typename Tqueue, typename Tobj>
	void test_mpmcu_queue(QueueType queueType, Tqueue& queue, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, int resultIndex)
	{
		//std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		mm::MM_HighResolutionClock::time_point start = mm::MM_HighResolutionClock::now();

		//Tqueue queue = createQueue_sfinae<Tqueue>(queueSize);
		const size_t threadsCount = numProducerThreads > numConsumerThreads ? numProducerThreads : numConsumerThreads;
		vector<std::thread> producerThreads;
		vector<std::thread> consumerThreads;
		int prodThreadId = -1;
		int consThreadId = -1;
		size_t numProdOperationsPerThread = numOperations / numProducerThreads;
		size_t numConsOperationsPerThread = numOperations / numConsumerThreads;
		for (size_t i = 0; i < threadsCount; ++i)
		{
			if (i < numProducerThreads)
				producerThreads.push_back(std::thread(producerThreadFunction<Tqueue, Tobj>, std::ref(queue), numProdOperationsPerThread, ++prodThreadId));
			if (i < numConsumerThreads)
				consumerThreads.push_back(std::thread(consumerThreadFunction<Tqueue, Tobj>, std::ref(queue), numConsOperationsPerThread, ++consThreadId));
		}

		for (size_t i = 0; i < numProducerThreads; ++i)
			producerThreads[i].join();

		for (size_t i = 0; i < numConsumerThreads; ++i)
			consumerThreads[i].join();

		//std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		mm::MM_HighResolutionClock::time_point end = mm::MM_HighResolutionClock::now();
		unsigned long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

		//cout << "      Queue is empty ? : " << (queue.empty() ? "Yes" : "No") << " Queue Size : " << queue.size();
		//cout << "\n    Total Duration: " << duration << " nanos."
		//	<< " Effective Duration: " << duration - (totalSleepTimeNanos / (numProducerThreads + numConsumerThreads)) << " nanos."
		//	<< " Total sleep time: " << totalSleepTimeNanos << " nanos." 
		//	<< " Average sleep time per thread: " << totalSleepTimeNanos / (numProducerThreads + numConsumerThreads) << " nanos.";
		results[resultIndex].result_[static_cast<int>(queueType)] = { queueType, duration, queue.empty(), queue.size() };
		string suffix{};
		if (!queue.empty() || queue.size() != 0)
		{
			suffix += "(";
			suffix += (queue.empty() ? "Y" : "N");
			suffix += to_string(queue.size());
			suffix += ")";
		}
		cout << std::setw(colWidth) << duration << suffix;
	}

	template<typename Tqueue, typename Tobj>
	struct is_fixed_size_queue
	{
		static const bool value = typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeQueue_v1<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeQueue_v2<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeQueue_v3<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5<Tobj>>::value
			|| typename std::is_same<Tqueue, MultiProducersMultiConsumersFixedSizeLockFreeQueue_v6<Tobj>>::value
			;
	};

	template<typename Tqueue, typename Tobj, typename std::enable_if<!is_fixed_size_queue<Tqueue, Tobj>::value, void>::type* = nullptr>
	void test_mpmcu_queue_sfinae(QueueType queueType, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, size_t queueSize, int resultIndex)
	{
		Tqueue queue{};
		test_mpmcu_queue<Tqueue, Tobj>(queueType, queue, numProducerThreads, numConsumerThreads, numOperations, resultIndex);
	}
	template<typename Tqueue, typename Tobj, typename std::enable_if<is_fixed_size_queue<Tqueue, Tobj>::value, void>::type* = nullptr>
	void test_mpmcu_queue_sfinae(QueueType queueType, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, size_t queueSize, int resultIndex)
	{
		Tqueue queue{ queueSize };
		test_mpmcu_queue<Tqueue, Tobj>(queueType, queue, numProducerThreads, numConsumerThreads, numOperations, resultIndex);
	}

	template<typename T>
	void runTestCase(int numProducerThreads, int numConsumerThreads, int numOperations, int queueSize, int resultIndex)
	{
		//size_t  = 25;
		//size_t  = 25;
		//size_t  = 50;
		//size_t  = 8; // numProducerThreads * numOperations / 10;

		//size_t total = (numProducerThreads + numConsumerThreads) * numOperations;
		sleepTimesNanosVec.reserve(numOperations);
		totalSleepTimeNanos = 0;
		for (size_t i = 0; i < numOperations; ++i)
		{
			size_t sleepTime = dist(mt64); // % 10000;
			totalSleepTimeNanos += sleepTime;
			sleepTimesNanosVec.push_back(sleepTime);
		}
		//totalSleepTimeNanos *= 1000ULL;

		columnNames.clear(); //Not a good fix! Make sure the columnNames are not pushed again and again for all test cases

		/***** Unlimited Queues ****/
		//The below queue crashes the program due to lack of synchronization
		//test_mpmcu_queue_sfinae<UnsafeQueue_v1<int>>("UNSAFE queue", numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex); 
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::vector<int>>>(numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::vector>>(numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);

		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<T>, T>(QueueType::MPMC_U_v1_deque, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<T, std::list>, T>(QueueType::MPMC_U_v1_list, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<T, std::forward_list>, T>(QueueType::MPMC_U_v1_fwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v2<T, std::list>, T>(QueueType::MPMC_U_v2_list, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v2<T, std::forward_list>, T>(QueueType::MPMC_U_v2_fwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v2<T, Undefined>, T>(QueueType::MPMC_U_v2_myfwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v3<T>, T>(QueueType::MPMC_U_v3_myfwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);

		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1<T>, T>(QueueType::MPMC_U_LF_v1, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v2<T>, T>(QueueType::MPMC_U_LF_v2, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		/*working but still need fix*/ test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v3<T>, T>(QueueType::MPMC_U_LF_v3, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		/*not working*/ test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v4<T>, T>(QueueType::MPMC_U_LF_v4, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v5<T>, T>(QueueType::MPMC_U_LF_v5, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v6<T>, T>(QueueType::MPMC_U_LF_v6, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		/*not working*/ //test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v7<T>, T>(QueueType::MPMC_U_LF_v7, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		/*not working*/ //test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v8<T>, T>(QueueType::MPMC_U_LF_v8, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);

		///***** Fixed Size Queues ****/
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<T>, T>(QueueType::MPMC_FS_v1, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v2<T>, T>(QueueType::MPMC_FS_v2, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v3<T>, T>(QueueType::MPMC_FS_v3, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);

		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<T>, T>(QueueType::MPMC_FS_LF_v1, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v2<T>, T>(QueueType::MPMC_FS_LF_v2, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v3<T>, T>(QueueType::MPMC_FS_LF_v3, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v4<T>, T>(QueueType::MPMC_FS_LF_v4, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		////test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v5<T>, T>(QueueType::MPMC_FS_LF_v5, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		////test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v6<T>, T>(QueueType::MPMC_FS_LF_v6, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);

		//The below queue does not work
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(QueueType::MPMC_FS_LF_vx, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
	}

	template<typename T>
	void runAllTestCasesPerType(int numOperations, bool useSleepLocal, int& resultIndex)
	{
		cout << "\n===============" << " Object Type: " << std::string{ typeid(T).name() } << " useSleep: " << useSleepLocal << " ===============";
		useSleep = useSleepLocal;
		std::vector<TestCase> tests =
		{
			{ 1, 1,     numOperations, 10 },
			{ 2, 2,     numOperations, 10 },
			{ 3, 3,     numOperations, 10 },
			{ 4, 4,     numOperations, 10 },
			{ 5, 5,     numOperations, 10 },
			{ 6, 6,     numOperations, 10 },
			{ 7, 7,     numOperations, 10 },
			{ 8, 8,     numOperations, 10 },
			{ 9, 9,     numOperations, 10 },
			{ 10, 10,   numOperations, 10 },
			{ 15, 15,   numOperations, 10 },
			{ 20, 20,   numOperations, 10 },
			{ 25, 25,   numOperations, 10 },
			{ 30, 30,   numOperations, 10 },
			{ 40, 40,   numOperations, 10 },
			{ 50, 50,   numOperations, 10 },
			{ 60, 60,   numOperations, 10 },
			{ 80, 80,   numOperations, 10 },
			{ 100, 100, numOperations, 10 },
		};

		results.insert(results.end(), tests.begin(), tests.end());

		for (int i = 0; i < tests.size(); ++i)
		{
			cout << "\n"
				<< std::setw(subCol1) << tests[i].numProducers_
				<< std::setw(subCol2) << tests[i].numConsumers_
				<< std::setw(subCol3) << tests[i].numOperations_
				<< std::setw(subCol4) << tests[i].queueSize_;

			runTestCase<T>(tests[i].numProducers_, tests[i].numConsumers_, tests[i].numOperations_, tests[i].queueSize_, ++resultIndex);
		}
	}

	MM_DECLARE_FLAG(Multithreading_mpmcu_queue);
	MM_UNIT_TEST(Multithreading_mpmcu_queue_test, Multithreading_mpmcu_queue)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		//int number = 123'456'789;
		//std::cout << "\ndefault locale: " << number;
		auto thousands = std::make_unique<separate_thousands>();
		std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
		//std::cout << "\nlocale with modified thousands: " << number;
		std::cout << std::boolalpha;

		//Print columns
		cout
			<< "\n\n"
			<< std::setw(firstColWidth) << "Test Case";

		for (int i = 0; i < static_cast<int>(QueueType::maxQueueTypes); ++i)
		{
			cout << std::setw(colWidth) << queueTypeToQueueName[static_cast<QueueType>(i)];
		}

		cout << "\n"
			<< std::setw(subCol1) << "Ps"
			<< std::setw(subCol2) << "Cs"
			<< std::setw(subCol3) << "TotOps"
			<< std::setw(subCol4) << "Qsz";

		int numOperations = 84000 * 4 * 4;
		int resultIndex = -1;
		int divFactor = 40 * 4;

		runAllTestCasesPerType<Object>(numOperations, false, resultIndex);
		runAllTestCasesPerType<int>(numOperations, false, resultIndex);
		runAllTestCasesPerType<int>(numOperations / divFactor, true, resultIndex);
		runAllTestCasesPerType<Object>(numOperations / divFactor, true, resultIndex);		
	}

}







	/// The below queue and its coresponding template specializations are not in use
	/*
	template<>
	void producerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<Object>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<Object>& queue, size_t numProdOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numProdOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				size_t sleepTime = sleepTimesNanosVec[threadId * numProdOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}
			int n = i;
			queue.push(std::move(Object{ n, n % 256 }), threadId);
		}
	}

	template<>
	void consumerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<Object>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<Object>& queue, size_t numConsOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numConsOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				size_t sleepTime = sleepTimesNanosVec[threadId * numConsOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}
			Object obj = std::move(queue.pop(threadId));
			int n = obj.getValue();
		}
	}

	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<Object>>(QueueType queueType, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, size_t queueSize, int resultIndex)
	{
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<Object> queue{ queueSize, numProducerThreads, numConsumerThreads };
		test_mpmcu_queue(queueType, queue, numProducerThreads, numConsumerThreads, numOperations, resultIndex);
	}
	*/


/*
Results:
without sleep time:

              Test Case     MPMC-U-v1-deque      MPMC-U-v1-list    MPMC-U-v1-fwlist        MPMC-U-LF-v1          MPMC-FS-v1       MPMC-FS-LF-v1
  Ps   Cs Ops/thread  Qsz

  1    1        1    1          10,512,800           5,097,400           5,619,400           4,938,300           4,758,900
  1    1      100   10           3,764,800           5,850,400           9,088,200           3,342,600           3,662,100
  2    2      100   10           6,814,900           6,649,000           7,908,300           8,334,500           6,703,300
  3    3      100   10          10,558,200          10,851,900          11,768,000          38,614,000           8,925,800
  4    4      100   10          15,684,400          17,696,200          16,985,400          14,359,100          12,815,600
  4    4      500   10          15,503,200          19,127,400          15,370,500          20,446,600          12,831,500
  5    5      100   10          14,342,200          14,654,900          16,962,500          13,110,000          13,066,800
  6    6      100   10          15,550,000          14,968,800          15,055,900          15,713,100          15,805,800
  7    7      100   10          18,838,900          17,118,400          27,866,400          32,619,300          46,323,900
  8    8      100   10          34,667,600          38,316,500          40,968,200          24,744,700          33,581,400
  9    9      100   10          46,101,800          44,937,800          22,793,300          54,987,200          32,195,900
 10   10      100   10          40,243,600          32,460,100          25,783,200          24,804,900          30,366,400
 10   10      500   10          38,266,300          22,844,900          21,782,700          20,834,000          22,432,700
 20   20    5,000   10          55,132,200          72,395,200          67,074,900          59,002,600          81,626,200
100  100  100,000   10       2,295,550,100       2,431,142,400       2,478,145,000       1,805,269,200       5,109,587,200

with sleep time:

             Test Case     MPMC-U-v1-deque      MPMC-U-v1-list    MPMC-U-v1-fwlist        MPMC-U-LF-v1          MPMC-FS-v1       MPMC-FS-LF-v1
 Ps   Cs  Ops/thread  Qsz
  
  1    1        1    1          92,102,700          10,272,600           6,010,200           3,792,700           6,103,700
  1    1      100   10          83,918,700         100,351,700          53,518,900          19,444,000          86,004,600
  2    2      100   10          34,781,400          92,310,700          63,236,300          95,707,000          23,135,100
  3    3      100   10          21,879,600          32,206,500          48,652,900          21,996,700          40,132,600
  4    4      100   10          56,095,700          44,570,000          80,877,500          99,548,200          83,371,200
  4    4      500   10         185,091,700         157,041,800         359,332,200          48,801,400          57,415,300
  5    5      100   10          36,853,100          33,966,900          35,237,300          58,058,000          37,894,300
  6    6      100   10          48,523,400          53,955,700          57,150,600          49,072,000          62,781,500
  7    7      100   10          72,806,100          74,893,300          53,730,900          50,012,300          51,488,200
  8    8      100   10          45,376,600          54,049,900         101,916,200          72,101,200          67,888,400
  9    9      100   10          48,577,500          95,669,300         108,464,700         119,415,700         127,087,700
 10   10      100   10          89,290,600         105,511,700          80,415,200          94,005,500          53,200,700
 10   10      500   10         127,712,600         118,717,600         128,169,700         103,525,000          96,378,100
 20   20    5,000   10       1,106,625,400       1,019,265,200       1,241,381,200         854,424,200       1,328,750,700
100  100  100,000   10      51,701,202,200      60,164,323,300      57,437,982,400      53,855,787,000      63,660,670,200

with sleep time:

             Test Case     MPMC-U-v1-deque      MPMC-U-v1-list    MPMC-U-v1-fwlist        MPMC-U-LF-v1          MPMC-FS-v1       MPMC-FS-LF-v1
  Ps   Cs  TotOps  Qsz
  
  1    1    8,400   10      13,832,868,200      13,592,742,300      13,478,902,000      13,728,150,100      14,196,506,500
  2    2    8,400   10       7,294,926,000       6,712,950,100       6,459,516,500       6,482,996,700       6,746,338,300
  3    3    8,400   10       4,534,152,600       4,435,520,300       4,534,132,100       4,598,716,100       4,653,351,300
  4    4    8,400   10       3,357,152,200       3,429,604,900       3,391,720,300       3,434,587,200       3,456,448,000
  5    5    8,400   10       2,766,419,700       2,807,537,900       2,780,383,100       2,782,261,800       2,875,267,600
  6    6    8,400   10       2,330,622,400       2,284,106,100       2,333,311,600       2,325,219,700       2,362,778,300
  7    7    8,400   10       2,001,412,100       1,987,384,100       2,008,673,000       2,042,688,900       1,985,367,000
  8    8    8,400   10       1,814,736,900       1,804,627,500       1,804,693,500       1,821,882,100       1,764,951,900
  9    9    8,400   10       1,639,738,600       1,618,763,000       1,613,716,000       1,548,838,300       1,698,784,100
 10   10    8,400   10       1,560,956,300       1,476,632,300       1,458,007,700       1,474,777,600       1,472,274,300
 20   20    8,400   10         780,777,000         831,739,600         844,885,500         848,771,200         844,838,800
 50   50    8,400   10         479,273,600         441,260,200         465,822,300         445,276,700         444,507,600
100  100    8,400   10         526,583,200         434,620,500         447,173,500         494,508,100         455,340,700



             Test Case     MPMC-U-v1-deque      MPMC-U-v1-list    MPMC-U-v1-fwlist  MPMC-U-v2-myfwlist        MPMC-U-LF-v1          MPMC-FS-v1
  Ps   Cs   TotOps  Qsz
  
  1    1    2,100   10       3,526,535,800       3,425,023,800       3,239,210,900       3,378,489,200       3,430,575,700       3,353,570,600
  2    2    2,100   10       1,683,799,000       1,589,979,400       1,659,027,300       1,713,031,900       1,705,787,100       1,690,169,300
  3    3    2,100   10       1,104,418,700       1,120,953,100       1,103,292,100       1,152,263,600       1,117,678,600       1,238,963,400
  4    4    2,100   10         861,775,700         902,676,400         914,569,300         865,973,200         900,788,300         879,610,700
  5    5    2,100   10         673,393,500         704,575,900         659,334,600         695,468,500         729,754,100         740,043,200
  6    6    2,100   10         660,255,800         640,265,100         586,447,100         606,560,600         592,222,400         639,309,000
  7    7    2,100   10         583,527,200         525,216,500         526,902,000         557,093,600         566,718,700         570,452,900
  8    8    2,100   10         460,388,600         503,199,400         464,088,600         502,845,300         509,734,000         513,877,200
  9    9    2,100   10         433,424,000         401,457,600         411,747,800         434,682,300         485,467,500         456,625,700
 10   10    2,100   10         413,122,600         412,532,000         367,249,900         427,021,100         382,729,300         434,349,800
 20   20    2,100   10         271,392,600         258,420,200         252,598,300         270,116,200         298,531,000         277,366,600
 50   50    2,100   10         239,467,100         241,762,000         216,692,700         231,421,800         252,337,000         232,998,000
100  100    2,100   10         427,344,200         338,983,000         321,968,700         313,574,300         344,150,500         317,002,800



             Test Case   MPMC_U_v1_deque    MPMC_U_v1_list  MPMC_U_v1_fwlistMPMC_U_v2_myfwlist      MPMC_U_LF_v1        MPMC_FS_v1
  Ps   Cs   TotOps  Qsz
  =============== with sleep time ===============
  1    1    2,100   10     3,461,297,800     3,302,488,800     3,284,398,900     3,100,012,400     3,093,153,400     3,240,099,600
  2    2    2,100   10     1,654,885,300     1,702,848,000     1,582,280,200     1,686,039,800     1,632,100,200     1,661,483,000
  3    3    2,100   10     1,188,781,900     1,274,385,800     1,174,244,500     1,122,139,900     1,138,466,100     1,164,724,800
  4    4    2,100   10       835,036,600       845,745,700       755,221,400       816,228,000       846,718,200       876,679,800
  5    5    2,100   10       654,947,800       736,660,700       701,814,800       646,446,700       787,969,800       859,932,500
  6    6    2,100   10       568,443,800       615,364,000       584,007,100       556,322,400       604,414,600     1,145,885,600
  7    7    2,100   10       516,939,300       509,552,800       558,587,900       519,710,400       474,935,400       690,395,700
  8    8    2,100   10       430,231,600       470,364,200       449,374,800       428,483,200       481,138,600       799,390,800
  9    9    2,100   10       439,170,300       369,148,700       399,009,400       414,466,800       427,647,300     1,836,160,000
 10   10    2,100   10       366,233,400       403,862,200       421,840,200       435,699,400       385,001,100     1,884,453,900
 20   20    2,100   10       356,239,300       261,795,400       222,409,500       242,469,700       264,913,400     1,064,114,500
 50   50    2,100   10       297,747,000       233,572,100       238,507,600       253,200,200       211,646,800       315,682,400
100  100    2,100   10       343,350,700       394,395,100       492,080,400       319,441,400       339,346,700     1,891,974,000
=============== without sleep time ===============
  1    1    2,100   10         4,802,200         5,187,400         5,528,200         5,479,100         3,450,900         4,190,100
  2    2    2,100   10         5,174,400         6,910,600         5,977,400         6,135,000         6,461,800         8,467,900
  3    3    2,100   10        10,188,000        10,254,600         8,681,400        10,455,600         7,808,500         8,499,500
  4    4    2,100   10         9,736,300        10,441,500        10,378,500        10,785,700        11,260,300        12,111,200
  5    5    2,100   10        12,944,600        11,838,400        13,784,800        12,134,300        12,670,700        15,000,700
  6    6    2,100   10        15,553,800        16,766,800        19,702,800        14,992,100        15,448,900        16,079,300
  7    7    2,100   10        16,241,800        17,371,100        17,146,100        65,204,100        33,190,200        27,211,700
  8    8    2,100   10        26,722,800        24,447,200        20,727,700        20,676,100        19,446,500        21,280,600
  9    9    2,100   10        24,131,200        23,353,800        21,312,400        22,842,400        22,659,500        24,258,700
 10   10    2,100   10        25,483,600        27,932,300        29,491,000        24,147,800        23,917,000        25,115,800
 20   20    2,100   10        48,092,900        48,061,000        50,742,000        50,650,900        56,154,100        52,132,000
 50   50    2,100   10       123,226,400       149,112,700       127,271,600       133,504,500       117,364,500       199,712,900
100  100    2,100   10       319,126,600       262,880,000       249,818,000       255,884,800       250,113,600       266,190,000








Release version:

           Test Case     MPMC_U_v1_deque      MPMC_U_v1_list    MPMC_U_v1_fwlist      MPMC_U_v2_list    MPMC_U_v2_fwlist  MPMC_U_v2_myfwlist        MPMC_U_LF_v1          MPMC_FS_v1
  Ps   Cs   TotOps  Qsz
=============== with sleep time ===============
  1    1    2,100   10       3,683,620,900       3,399,925,600       3,499,378,900       3,589,693,300       3,462,244,900       3,321,517,600       3,459,206,600       3,926,187,700
  2    2    2,100   10       1,809,628,000       1,779,192,400       1,738,315,600       1,725,375,200       1,660,659,300       1,625,149,000       1,618,697,100       1,993,083,700
  3    3    2,100   10       1,190,887,900       1,128,281,500       1,140,282,000       1,113,509,400       1,126,932,800       1,158,316,800       1,128,220,700       1,157,545,900
  4    4    2,100   10         839,778,600         884,739,500         872,817,600         908,601,300         806,205,000         849,085,500         842,905,100         847,789,500
  5    5    2,100   10         715,350,400         727,091,100         696,549,800         698,722,700         746,408,800         723,977,600         686,570,000         767,329,200
  6    6    2,100   10         620,719,000         618,424,100         636,101,700         635,093,400         600,242,400         642,961,700         612,276,700         595,388,600
  7    7    2,100   10         571,692,400         520,295,200         551,962,100         503,212,300         555,442,100         553,317,800         540,763,700         529,889,300
  8    8    2,100   10         421,776,900         441,177,000         459,177,400         432,656,300         491,548,900         489,477,500         509,434,500         489,434,400
  9    9    2,100   10         460,232,800         409,029,100         485,127,500         446,931,400         481,516,900         423,613,200         424,066,600         475,794,900
 10   10    2,100   10         386,390,800         448,403,300         340,297,700         388,509,700         381,232,700         415,124,700         430,844,000         419,830,700
 20   20    2,100   10         272,051,300         241,721,300         249,912,800         239,103,700         214,403,400         247,919,700         253,184,400         266,464,000
 50   50    2,100   10         246,343,000         240,357,000         225,574,100         232,889,800         234,556,600         238,690,700         228,581,200         219,140,500
100  100    2,100   10         429,567,100         330,448,000         298,657,300         309,864,200         327,597,600         310,523,000         297,721,400         444,783,700
=============== without sleep time ===============
  1    1   84,000   10          12,226,600          15,470,300          15,424,000          18,389,000          17,823,600          17,980,400          20,204,500          26,818,300
  2    2   84,000   10          14,890,700          22,948,000          23,128,600          24,953,200          26,588,500          23,493,600          17,841,500          36,847,800
  3    3   84,000   10          16,402,600          22,822,800          26,307,800          29,805,000          31,110,800          29,559,500          19,883,300          38,732,500
  4    4   84,000   10          19,486,700          26,011,800          26,548,300          33,119,900          46,665,700          37,558,100          26,141,800          42,181,300
  5    5   84,000   10          21,025,000          27,537,900          42,859,100          37,886,900          37,324,300          34,502,800          23,245,800          43,460,200
  6    6   84,000   10          22,255,300          30,142,300          35,769,800          37,220,600          39,813,000          36,514,600          24,560,100          44,299,100
  7    7   84,000   10          25,480,600          38,458,100          40,691,400          43,546,900          41,027,000          38,029,500          26,411,500          60,032,200
  8    8   84,000   10          30,599,900          40,728,700          37,898,900          44,116,700          55,883,000          91,565,700          67,927,900          69,564,200
  9    9   84,000   10          31,348,700          37,480,000          41,533,000          46,872,000          48,754,800          43,977,700          33,048,600          52,801,900
 10   10   84,000   10          36,091,800          45,764,700          43,264,400          51,636,300          48,707,100          43,866,300          38,053,600          59,309,800
 20   20   84,000   10          52,354,700          61,399,500          67,326,600          77,196,300          64,009,700          64,670,000          59,963,400          88,501,900
 50   50   84,000   10         117,956,900         154,439,100         127,754,200         154,063,100         132,495,100         229,077,400         197,213,800         190,575,900
100  100   84,000   10         247,816,100         247,725,100         267,335,000         262,256,800         262,928,400         250,687,900         335,150,100         356,562,200

Execution of unit tests is finished. All tests have passed unless any failure printed above!



Debug Version:

           Test Case     MPMC_U_v1_deque      MPMC_U_v1_list    MPMC_U_v1_fwlist      MPMC_U_v2_list    MPMC_U_v2_fwlist  MPMC_U_v2_myfwlist        MPMC_U_LF_v1          MPMC_FS_v1
  Ps   Cs   TotOps  Qsz
=============== with sleep time ===============
  1    1    2,100   10         909,311,400         348,020,800         138,537,600          72,283,600         110,045,800          64,669,400          65,955,100          79,490,900
  2    2    2,100   10          75,225,000         118,246,500         191,053,300          86,354,500         153,106,600          63,307,700         260,628,200          30,522,000
  3    3    2,100   10         154,167,900         242,663,600         219,019,400          95,413,000         168,852,800          35,933,700         122,120,400          45,433,100
  4    4    2,100   10          94,528,900         190,940,900         160,311,200         133,142,500         129,733,400          83,772,400         265,005,000          77,730,500
  5    5    2,100   10          72,197,600         146,313,500         179,031,300         128,519,000         131,839,500          46,451,800         150,995,400          65,017,300
  6    6    2,100   10         101,543,200         125,542,400         139,847,300         151,431,600         152,734,400          88,329,400          48,936,300          44,710,000
  7    7    2,100   10         104,990,100         204,881,600         145,104,800         109,659,800         168,322,700          55,848,700          64,590,200          51,457,400
  8    8    2,100   10         130,282,500         148,282,900         150,013,100         131,950,200         120,451,600          65,823,000         150,857,800          83,083,800
  9    9    2,100   10          87,291,000         148,097,200         151,779,500         181,599,100         172,424,200         108,582,700         111,013,300          55,807,000
 10   10    2,100   10          92,901,400         171,571,400         159,570,100         113,674,600         132,388,100          92,417,500          68,643,000         113,206,300
 20   20    2,100   10         110,700,500         153,689,000         160,769,400         140,499,800         147,027,400         147,810,800         122,926,900         135,835,200
 50   50    2,100   10         159,539,200         179,180,300         195,584,900         182,036,400         192,239,200         190,574,100         257,567,700         181,097,200
100  100    2,100   10         386,841,800         407,825,600         297,297,100         318,696,200         332,039,200         294,154,400         846,063,800         343,511,400
=============== without sleep time ===============
  1    1   84,000   10         312,136,600         409,164,100         518,658,500         379,248,600         570,001,400          98,475,000          79,832,300         189,409,100
  2    2   84,000   10         333,458,100         708,029,300         803,184,200         402,957,300         566,072,300         108,791,300          78,829,400         203,640,500
  3    3   84,000   10         312,936,800         692,563,500         930,100,500         451,820,200         869,072,200         113,073,300          98,066,800         177,536,800
  4    4   84,000   10         283,995,500         795,380,100         854,363,300         555,113,900         810,624,900         129,191,000          98,263,300         188,914,200
  5    5   84,000   10         288,061,000         803,721,000         956,030,000         594,221,100         837,845,900         123,353,700         101,669,700         190,960,600
  6    6   84,000   10         290,509,100         821,942,800         919,889,700         578,091,100         903,784,900         141,087,000         129,794,100         192,399,300
  7    7   84,000   10         286,821,500         898,633,300         906,885,500         609,230,700         901,282,000         124,360,100         112,249,800         199,583,100
  8    8   84,000   10         316,946,200         930,875,000         981,047,700         598,675,500         925,002,000         128,391,800         117,545,600         205,448,200
  9    9   84,000   10         323,840,800         817,798,800         952,310,100         622,442,400         869,179,900         132,069,400         118,031,600         206,058,800
 10   10   84,000   10         314,212,600         958,737,200         964,776,000         618,761,600       1,011,783,900         163,706,600         132,609,300         202,184,100
 20   20   84,000   10         320,011,600         996,587,600         938,486,500         665,872,100         940,121,200         285,591,000         257,190,100         247,765,300
 50   50   84,000   10         406,838,100       1,108,178,500       1,241,982,100         799,175,900       1,085,542,300         303,309,600         239,808,800         369,940,400
100  100   84,000   10         680,874,200       1,244,480,800       1,348,426,000         910,571,000       1,189,302,300         489,589,300         382,611,500         439,472,300

Execution of unit tests is finished. All tests have passed unless any failure printed above!


Executing unit tests...



            Test Case     MPMC_U_v1_deque      MPMC_U_v1_list    MPMC_U_v1_fwlist      MPMC_U_v2_list    MPMC_U_v2_fwlist  MPMC_U_v2_myfwlist  MPMC_U_v3_myfwlist        MPMC_U_LF_v1        MPMC_U_LF_v2        MPMC_U_LF_v3        MPMC_U_LF_v4          MPMC_FS_v1          MPMC_FS_v2          MPMC_FS_v3       MPMC_FS_LF_v1       MPMC_FS_LF_v2       MPMC_FS_LF_v3       MPMC_FS_LF_v4       MPMC_FS_LF_v5
 Ps   Cs   TotOps  Qsz
 =============== with sleep time ===============
 1    1    2,100   10       1,443,331,300       1,368,220,000       1,335,870,800       1,244,659,500       1,218,594,900       1,781,376,600       1,936,361,100         959,080,700       1,051,024,600         952,502,600       1,133,218,400       1,328,941,900       1,276,320,900       1,181,429,000       1,098,061,000       1,041,256,800       1,157,345,400         919,067,700       1,074,115,300
 2    2    2,100   10       1,002,106,600         867,333,800         952,029,700         844,937,200         855,649,700       1,180,674,200       1,220,533,700         711,933,900         726,856,800         573,456,400         596,408,700         778,375,300         869,083,500         693,006,500         597,323,700         700,583,600         600,184,600         655,437,100         715,286,000
 3    3    2,100   10         781,866,100         696,917,200         754,880,200         695,163,400         692,843,400       1,053,329,900         950,259,500         567,471,600         578,153,400         437,108,800         449,006,200         785,255,600         632,624,800         604,751,500         487,352,300         466,375,900         520,153,800         478,109,300         559,408,500
 4    4    2,100   10         702,095,100         914,759,100         818,263,300         710,402,700         794,003,800       1,396,324,300       1,311,172,700         438,630,600         434,008,300         397,984,100         637,905,400         678,741,800         524,781,700         416,049,600         971,319,200       1,309,245,700         761,527,600         474,042,300         680,330,300
 5    5    2,100   10         620,335,500         583,246,800         557,387,600         502,851,400         507,725,300       1,105,883,800         776,363,100         488,211,700         402,877,300         319,159,600         370,651,100         547,236,400         531,875,800         375,550,500         582,294,500         634,341,900         900,789,100         660,218,200         626,587,700
 6    6    2,100   10         600,277,500         596,610,300         552,240,400         493,991,300         549,209,600       1,121,651,000         791,305,800         528,829,400         392,967,600         284,746,100         348,825,300         525,718,400         523,466,400         366,445,700         757,518,900         894,142,400       2,292,456,000         660,435,900         625,614,300
 7    7    2,100   10         554,774,300         556,655,700         618,528,500         485,606,000         515,179,400       1,136,532,600         810,980,200         372,431,400         341,330,600         381,202,800         299,726,500         530,309,700         464,038,900         307,735,700       1,109,547,000       3,619,693,200         505,084,000         571,197,400         629,406,300
 8    8    2,100   10         564,622,300         585,586,100         594,873,500         472,176,300         504,632,200       1,071,718,800         795,232,000         463,740,600         511,623,900         382,378,700         315,538,700         540,612,000         447,108,100         363,454,900       1,057,220,900       3,088,478,000      10,720,260,200         711,168,400         883,104,600
 9    9    2,100   10         644,370,800         535,862,400         536,271,800         477,360,000         600,830,100       1,001,506,100         803,598,900         575,009,400         443,641,000         308,449,100         235,655,700         521,767,400         532,840,400         288,277,000       5,464,993,800      13,884,099,800      13,229,274,800         370,321,000         727,075,300
10   10    2,100   10         665,889,800         572,275,900         539,839,600         654,388,700         767,936,700       1,213,568,400       1,005,156,400         782,528,800         729,150,700         268,785,600         324,595,600         646,591,100         578,055,600         332,650,300       9,659,391,200      13,754,971,400      30,993,682,600         450,754,200       1,081,410,000
20   20    2,100   10         793,553,000         605,063,100         593,980,400         590,262,200         612,155,700       1,018,473,800         906,775,600       2,991,741,200       2,758,182,800         534,772,400         383,405,300         570,735,700         499,716,400         314,365,800     135,449,203,900     108,332,600,700     106,958,971,700         726,293,900      11,650,287,900
50   50    2,100   10         757,171,200         849,857,900         708,527,700         632,384,700         634,276,600       1,215,276,400         844,097,400       4,760,253,500       5,707,919,900         478,981,400         325,706,600         620,358,100         538,970,800         458,554,800     339,801,519,700     232,749,862,700






Executing unit tests...



  Test Case                MPMC_U_v1_deque      MPMC_U_v1_list    MPMC_U_v1_fwlist      MPMC_U_v2_list    MPMC_U_v2_fwlist  MPMC_U_v2_myfwlist  MPMC_U_v3_myfwlist        MPMC_U_LF_v1        MPMC_U_LF_v2        MPMC_U_LF_v3        MPMC_U_LF_v4          MPMC_FS_v1          MPMC_FS_v2          MPMC_FS_v3       MPMC_FS_LF_v1       MPMC_FS_LF_v2       MPMC_FS_LF_v3       MPMC_FS_LF_v4       MPMC_FS_LF_v5       MPMC_FS_LF_v6
  Ps   Cs   TotOps  Qsz
  =============== with sleep time ===============
  1    1    2,100   10      20,431,226,300       8,960,458,700      22,586,654,200      23,633,914,900      22,631,569,700      23,467,285,800      24,231,574,600      23,181,660,400      23,727,087,600      23,767,157,500      23,088,812,200      24,542,589,200      23,441,375,700      24,333,240,100      22,985,513,600      23,002,544,000      24,538,059,500      23,334,710,300      25,567,027,900      24,345,168,000
  2    2    2,100   10      11,916,859,000      12,044,588,100      11,958,434,900      11,995,176,200      12,201,827,300      12,520,598,000      12,783,982,600      12,371,399,500      12,008,309,300      13,348,263,700      11,949,506,000      12,005,051,500      13,043,687,600      12,520,325,300      12,124,924,100      12,840,099,800      12,552,958,800      11,718,965,600      12,970,886,600      12,218,829,200
  3    3    2,100   10       8,450,638,700       8,761,918,100       8,323,440,600       7,753,253,500       8,552,731,400       8,721,337,000       8,292,792,000       8,588,492,300       8,233,335,300       8,868,864,100       8,608,887,100       8,379,584,700       7,736,609,700       9,115,831,000       9,600,628,300       8,980,485,300       8,526,779,400       9,137,017,600       8,888,574,700       8,239,694,000
  4    4    2,100   10       6,703,737,100       6,269,843,700       6,726,256,100       6,515,964,000       6,005,693,700       5,954,047,500       5,680,465,100       7,032,489,100       6,510,495,800       6,476,883,200       6,450,287,200       6,600,353,000       6,697,491,300       5,860,583,100       6,701,719,100       6,640,410,100       6,913,635,800       6,996,982,700       6,636,752,800       6,764,441,500
  5    5    2,100   10       5,110,548,300       5,026,807,000       5,145,204,600       4,887,882,700       4,992,416,500       5,256,004,900       5,312,411,600       6,101,022,300       5,526,908,600       5,286,820,900       5,529,821,800       5,294,669,600       5,527,161,800       5,093,933,800       5,492,639,900       5,990,148,300       5,937,578,700       5,851,884,000       5,605,685,200       6,039,650,700
  6    6    2,100   10       4,307,803,400       4,266,042,100       4,445,010,000       4,392,356,500       4,640,923,200       4,720,577,400       4,735,036,700       5,085,810,400       5,170,227,800       4,589,414,400       5,195,960,400       4,751,896,100       4,475,287,600       4,416,337,700       5,248,686,100       5,248,576,000       5,400,862,700       4,920,624,800       5,058,487,500       4,800,816,600
  7    7    2,100   10       3,354,254,600       3,358,341,900       3,453,209,900       3,871,352,700       4,166,053,300       4,418,870,300       4,170,882,400       4,345,091,300       4,301,906,700       4,061,545,200       4,303,459,300       3,828,176,400       3,787,531,800       3,736,355,800       4,308,179,000       5,189,086,000       4,418,658,900       4,413,528,900       5,271,766,700       4,710,874,900
  8    8    2,100   10       3,093,815,000       3,381,001,600       3,147,486,500       3,279,415,700       3,278,228,300       3,545,738,100       3,418,260,800       4,073,087,500       3,744,819,100       3,805,648,400       4,247,429,800       3,252,820,800       3,557,351,600       3,404,015,400       4,244,684,000       4,311,480,200       3,966,006,100       4,326,189,700       9,053,716,500       4,228,681,900
  9    9    2,100   10       3,132,840,300       3,305,078,300       2,881,246,600       3,174,433,400       2,843,959,400       3,163,157,300       2,937,760,700       4,051,548,500       3,585,768,100       3,396,408,500       3,413,438,400       2,969,954,300       3,228,228,900       3,070,252,200       3,670,303,200       3,987,241,900       4,116,824,800       3,746,920,900      32,578,783,300      31,618,641,400
 10   10    2,100   10       2,981,629,800       2,922,786,500       2,741,962,800       2,817,755,300       2,720,055,700       3,129,303,900       2,372,508,500       3,455,737,900       3,331,815,200       3,294,798,500       2,960,379,400       3,059,239,500       3,139,766,700       2,795,114,100       3,861,574,200       4,218,032,700       3,840,255,200       6,785,331,200      36,260,970,100      61,583,625,200
 15   15    2,100   10       2,123,500,400       2,138,695,000       2,079,646,300       2,058,105,200       2,006,620,300       2,188,062,000       2,101,782,300       2,796,546,600       2,744,946,600       2,501,276,700       2,721,388,500       2,009,613,100       1,873,516,400       1,891,025,000      13,305,621,100       2,329,305,100       2,987,710,600       7,059,486,900      96,615,975,900     159,194,435,400
 20   20    2,100   10       1,585,233,100       1,633,331,700       1,588,152,700       1,506,247,700       1,569,825,200       1,712,855,300       1,573,866,000       2,307,327,100       2,423,630,900       1,761,353,600       1,887,468,000       1,646,584,000       1,539,640,500       1,415,101,300      31,045,550,100       2,814,874,900       2,672,198,400      20,244,342,500     123,035,018,800     240,790,210,500
 25   25    2,100   10       1,119,401,900       1,258,216,200       1,149,592,400       1,313,761,500       1,336,202,700       1,451,210,100       1,250,263,000       3,127,106,900       4,077,786,200       1,513,398,000       2,330,963,200       1,344,192,300       1,317,879,000       1,326,143,600      15,364,412,500       2,804,707,700       2,092,922,100      16,255,317,400     360,431,656,300     318,481,480,100
 30   30    2,100   10       1,166,040,000       1,147,219,100       1,136,494,100       1,172,288,500       1,181,111,400       1,123,193,600       1,172,809,000       3,086,769,300       3,123,393,600       2,265,410,200       1,684,927,000       1,098,228,100       1,209,386,200       1,032,239,800      53,212,592,400       1,552,812,700       1,419,672,600      58,417,955,800     431,688,948,900     455,642,996,900
 40   40    2,100   10         935,832,600         919,203,700         978,976,000         968,914,800       1,065,728,000       1,052,501,000       1,040,447,000       7,763,427,500       9,346,188,600       1,398,855,600       1,572,633,000         890,825,100         920,140,600         830,268,300     136,521,549,800       2,053,167,400       1,514,258,300     408,445,649,700     492,075,552,100     462,621,604,700
 50   50    2,100   10         763,747,200         823,247,100         827,075,800         857,879,800         935,742,700       1,021,235,100         801,985,600      10,560,466,900       1,632,492,900       7,821,582,100       1,032,228,400         840,613,100         828,416,700         809,647,500      56,480,238,000       4,233,574,800       1,922,418,200      38,068,615,700     914,389,654,700     476,546,450,800
 60   60    2,100   10         823,150,800         825,683,800         749,177,300         795,615,700         742,957,700         857,088,700         831,675,500      20,870,744,500      20,502,174,400       3,491,783,600         691,250,500         763,576,000         719,365,800         733,368,700     228,738,475,700      13,154,730,700       1,515,830,300      53,558,859,300     944,334,116,800   1,010,835,093,800
 80   80    2,100   10         652,442,000         615,764,000         603,640,800         601,389,900         591,643,400         774,716,200         711,549,400      49,693,108,100      44,301,866,900       2,663,047,900       1,104,986,000         827,471,400         655,788,000         619,909,200     241,473,027,500      31,276,366,800       4,532,152,200     589,596,087,500     498,913,972,600     890,102,697,800
100  100    2,100   10         683,214,400         645,523,400         618,083,200         727,412,700         650,883,600         871,058,400         740,037,500      62,776,624,300      80,889,816,700      33,109,882,800         967,731,100         578,037,600         589,322,300         516,191,500      43,910,917,100      58,897,904,200       1,481,946,000









             Test Case     MPMC_U_v1_deque      MPMC_U_v1_list    MPMC_U_v1_fwlist      MPMC_U_v2_list    MPMC_U_v2_fwlist  MPMC_U_v2_myfwlist  MPMC_U_v3_myfwlist        MPMC_U_LF_v1        MPMC_U_LF_v2        MPMC_U_LF_v3        MPMC_U_LF_v4          MPMC_FS_v1          MPMC_FS_v2          MPMC_FS_v3       MPMC_FS_LF_v1       MPMC_FS_LF_v2       MPMC_FS_LF_v3       MPMC_FS_LF_v4       MPMC_FS_LF_v5       MPMC_FS_LF_v6
  Ps   Cs   TotOps  Qsz
  =============== useSleep: false ===============
  1    1   84,000   10         110,837,100          78,968,700          72,059,300          63,616,900          53,762,400          63,631,300          41,434,200          53,410,100          31,208,700          40,542,300          33,826,000          45,534,000          44,777,400          41,398,200          39,159,600          28,884,000          42,339,100          42,409,300          28,622,000          32,337,200
  2    2   84,000   10          44,222,300          55,358,900          53,769,300          60,963,900          59,471,700          55,437,700          41,001,200          28,753,700          23,379,100          34,200,400          23,893,800          60,858,900          75,636,200          42,735,100          32,863,900          22,042,000          23,985,700          22,622,200          21,604,400          22,813,300
  3    3   84,000   10          45,008,500          63,449,700          54,217,600          60,079,500          80,926,200          82,904,800          66,177,600          66,880,400          48,610,600          51,784,600          42,480,000          66,900,800          81,537,400          41,040,100          35,334,700          31,440,400          35,944,400          28,227,600          33,669,500          28,931,500
  4    4   84,000   10          44,898,400          61,477,200          59,948,500          69,364,800          63,191,600          71,026,500          45,843,100          32,134,400          28,276,900          28,062,700          28,170,500          73,078,700          68,429,800          41,252,700          34,619,100          46,826,900          42,666,600          28,551,200          29,198,300          29,084,500
  5    5   84,000   10          49,579,800          64,050,000          60,907,700          71,560,700          74,626,900          76,906,800          50,726,100          44,472,700          37,362,100          35,736,100          32,954,500          72,196,200          97,377,100          59,055,600          89,230,300         101,972,800          39,731,900          33,183,400          52,120,000          52,390,400
  6    6   84,000   10          51,068,000          61,810,500          67,157,000          77,155,500          71,292,300          76,324,100          52,105,400          37,348,000          31,436,300          28,983,700          31,242,400          77,459,400          81,473,900          45,944,500          33,935,000          39,061,800          39,871,800          31,750,400          37,081,900          33,610,300
  7    7   84,000   10          57,601,500          73,713,300          67,750,500          81,399,200          83,726,900          85,023,700          60,254,700          47,876,200          36,200,600          40,982,400          35,941,500          73,857,600          95,063,900          76,991,300         117,185,200         126,318,100          74,405,500          46,367,100          45,486,100          46,284,500
  8    8   84,000   10          57,329,300          70,538,100          65,283,500          76,816,600          71,084,500          76,144,700          59,760,600          74,225,500          48,686,400          51,710,900          75,073,300         107,406,600          93,531,600          58,502,400          38,968,500          38,172,300          42,918,900          43,076,500          53,085,900          41,732,900
  9    9   84,000   10          57,988,200          70,798,500          67,493,300          76,639,300          70,004,500          73,333,500          58,637,100          56,879,400          73,092,100          79,076,700          74,857,800          97,616,400          87,827,700          53,310,400          50,219,500          63,448,600          47,989,800          50,732,100          47,349,000          61,109,800
 10   10   84,000   10          64,885,300          75,841,500          74,064,600          78,885,700          75,626,500          81,067,000          61,999,600          50,878,900          43,174,300          40,888,600          52,226,400          88,194,900          99,507,600          63,645,100          45,424,700          43,465,200          49,122,700          77,951,600          73,355,900          48,217,900
 15   15   84,000   10          77,120,200          86,544,200         108,834,300         152,214,700         117,495,300          88,921,800          67,440,200          58,224,900          61,167,600          54,761,500          53,217,000         105,729,500         100,381,600          68,312,000          70,584,600          63,634,600          70,307,600          75,664,500          65,739,700          53,571,000
 20   20   84,000   10          83,951,100         106,508,300          96,392,100         101,306,100          99,792,800          98,404,800          80,519,800          68,234,700          68,367,500          69,933,300          76,224,800         161,113,500         152,980,400          92,257,300          71,809,300          71,185,500          89,752,100          70,194,500          68,707,900          85,981,000
 25   25   84,000   10          88,609,600         107,233,500          97,687,200         112,753,300         105,824,500         106,785,000          93,990,500          76,259,900          72,421,800          87,397,100          71,738,800         122,553,500         133,595,900         112,953,600         133,670,600         199,723,100         102,893,900          81,011,100          75,657,800         102,091,400
 30   30   84,000   10         100,973,200         121,678,800         125,220,400         116,531,900         116,586,700         126,235,000         123,821,500          89,392,600         100,454,300          85,504,300          85,867,200         146,652,000         150,245,200          99,776,800         122,332,800         210,377,400         125,540,200          88,165,100         129,824,900         103,265,600
 40   40   84,000   10         137,731,300         137,446,100         136,968,700         139,205,600         137,269,300         142,727,200         137,542,700         122,104,300         107,725,300         112,645,900         115,254,600         177,243,700         253,249,200         170,378,100         112,101,700         143,996,900         123,339,000         142,220,500         111,297,200         136,587,500
 50   50   84,000   10         149,515,700         169,663,700         180,694,200         165,308,100         170,126,000         158,745,800         204,396,400         253,275,000         131,867,300         191,767,800         169,755,600         175,749,800         188,318,700         157,547,200         132,248,800         139,085,800         150,340,500         142,626,300         169,763,200         147,745,600
 60   60   84,000   10         279,823,400         246,765,900         190,805,400         196,651,100         179,214,500         185,769,900         180,334,800         175,577,500         173,251,700         148,984,300         174,261,500         187,855,700         309,066,700         247,112,500         182,274,500         158,493,500         174,448,000         216,435,700         218,523,100         164,970,800
 80   80   84,000   10         209,103,800         235,838,700         250,302,600         399,741,200         230,674,200         219,786,400         222,021,200         216,282,000         206,604,100         198,002,500         200,799,900         313,970,500         392,130,700         234,520,900         220,093,800         217,690,200         358,016,500         195,750,500         233,222,200         240,243,900
100  100   84,000   10         278,838,200         291,082,600         282,487,700         283,101,900         428,567,900         289,640,500         334,694,100         372,162,000         286,167,500         288,944,400         305,002,500         458,734,200         316,974,100         327,865,800         299,346,900         270,153,300         270,205,800         300,048,600         336,190,300         376,067,100
=============== useSleep: false ===============
  1    1   84,000   10           9,542,800          16,019,700          19,001,300          17,457,300          16,365,000          15,686,600          19,162,400          26,778,500          14,995,900          15,121,600          17,356,200          29,349,100          43,099,400           8,753,500          12,355,400          14,394,300          21,988,300          13,018,300          18,736,000          18,031,100
  2    2   84,000   10          12,757,600          20,936,600          21,686,200          24,064,900          21,137,800          21,155,900          21,310,300          18,603,700          15,135,000          20,570,800          18,410,600          37,397,600          53,391,300          21,969,800          30,669,600          16,615,300          16,971,400          18,615,400          27,428,000          14,127,400
  3    3   84,000   10          16,838,600          26,209,000          26,077,900          28,983,800          28,943,400          25,030,400          22,275,200          28,090,400          19,438,900          18,579,900          18,667,800          36,741,100          58,906,100          22,272,500          62,217,900          21,364,600          25,012,200          17,517,400          16,693,500          25,975,700
  4    4   84,000   10          16,557,200          25,967,900          27,466,400          33,674,800          29,917,800          28,058,200          23,591,000          21,147,400          16,312,300          19,384,200          18,511,700          40,911,000          63,464,800          29,541,200          33,346,000          38,675,800          58,289,400          81,907,900          60,194,900          18,308,300
  5    5   84,000   10          18,040,900          28,307,900          29,762,100          31,735,800          31,470,200          33,795,800          26,971,400          23,953,000          19,927,500          20,611,400          22,691,500          44,835,700          59,821,900          26,890,500          33,214,400          26,369,100          27,015,000          32,046,100          30,930,500          21,968,800
  6    6   84,000   10          20,026,200          33,956,400          34,927,400          36,819,900          47,592,800          44,656,800          32,061,600          27,238,900          23,814,500          22,079,100          22,228,300          47,569,000          70,036,000          31,708,500          42,383,000          29,288,400          28,879,400          43,418,600          21,969,200          22,406,600
  7    7   84,000   10          22,939,100          32,124,800          30,954,300          36,950,100          39,448,400          40,843,000          36,258,800          28,646,900          28,698,200          29,898,100          27,311,500          50,382,400          73,304,800          32,163,900          38,838,500          24,987,200          32,075,200          33,383,000          83,350,400          73,181,300
  8    8   84,000   10         123,448,800          37,909,700          32,445,300          38,643,400          42,951,900          40,131,900          35,320,200          31,246,200          26,797,900          27,654,500          28,603,300          53,494,200          70,194,000          35,161,700          43,118,700          29,631,900          37,692,700          36,945,000          26,518,000          30,568,600
  9    9   84,000   10          31,595,600          35,953,400          38,344,900          42,761,700          51,367,900          40,883,000          33,671,300          35,036,400          27,766,400          27,404,300          39,423,900          55,806,800          70,803,300          37,204,400          42,338,400          35,794,300          38,582,000          31,196,200          29,211,700          29,542,000
 10   10   84,000   10          31,483,500          38,550,700          43,788,900          52,606,700          42,357,100          45,246,300          41,096,600          37,642,400          38,055,200          43,116,700          36,787,300          67,631,800         108,433,700          63,865,100          63,783,800          74,158,400          39,303,300          32,209,200          31,468,700          40,924,000
 15   15   84,000   10          41,910,500          53,819,500          53,913,400          54,191,700          53,239,000          62,442,900          51,100,200          57,711,200          41,442,000          43,766,100          44,737,200          86,620,400          86,568,000          59,018,600          81,019,500          43,410,600          49,547,000          43,680,800          52,740,300          51,841,700
 20   20   84,000   10          49,501,500          59,001,500          62,296,500          71,722,700          72,467,100          64,153,700          62,712,900          57,739,700          49,610,400          51,855,000          55,507,600         103,949,300         136,083,700         129,447,100          69,880,200          59,317,600          66,305,300          67,140,800          57,233,100          81,762,000
 25   25   84,000   10          72,009,300          74,653,400          80,150,200          88,589,700          72,976,200          81,318,800          72,958,900          73,850,300          68,447,600          76,436,000          72,732,400         101,071,200         125,644,800          70,284,900          83,963,800          83,354,200          74,736,900         102,217,800          81,384,100         116,281,700
 30   30   84,000   10         152,398,600         132,073,000          87,135,400          93,115,100         101,463,000          87,509,700          89,456,200          99,739,000          68,140,700          71,434,700          77,666,400         118,384,200         124,234,200          85,407,400          82,903,200          88,804,000          82,961,000          75,709,900         100,422,500          77,926,200
 40   40   84,000   10          95,465,900         116,884,700          99,847,700         184,754,700         176,262,000         107,910,000          97,531,700         121,608,300          91,521,000          99,538,700          99,166,300         148,466,200         146,436,900         119,671,000         128,452,400         153,082,400          96,584,200         100,567,100         129,568,000         109,349,200
 50   50   84,000   10         130,718,500         206,230,800         245,455,800         122,431,300         135,844,400         122,431,900         141,448,400         130,362,400         130,457,400         109,339,700         124,408,500         134,240,700         193,441,000         121,687,900         130,794,700         117,869,000         130,673,400         115,636,100         194,069,400         259,536,700
 60   60   84,000   10         141,071,400         148,283,500         153,102,700         149,642,500         140,798,200         159,373,100         132,215,700         165,404,600         136,603,800         154,289,000         134,352,400         166,157,600         192,084,000         283,129,400         165,239,100         148,383,100         155,202,700         144,512,700         226,237,900         149,230,200
 80   80   84,000   10         174,720,300         195,420,900         240,304,400         188,857,000         180,250,300         212,343,500         192,094,400         191,049,100         197,539,000         188,789,400         203,774,600         352,182,800         246,714,200         200,264,500         188,947,900         193,330,500         200,910,500         188,602,300         193,424,000         190,646,900
100  100   84,000   10         231,824,100         273,674,300         226,561,300         283,184,800         233,348,800         312,078,600         282,952,200         270,945,600         234,690,000         234,766,800         231,924,100         265,746,200         308,865,600         243,608,800         361,399,600         321,725,500         263,950,800         271,000,100         267,720,900         242,401,300
=============== useSleep: true ===============






*/