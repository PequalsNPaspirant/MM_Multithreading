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
using namespace std;

#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx.h"
#include "MultiProducersMultiConsumersFixedSizeQueue_v1.h"

#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v2.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v2.h"

#include "MultiProducersMultiConsumersUnsafeQueue_v1.h"

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

#define USE_SLEEP

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
		MPMC_U_LF_v1,
		MPMC_FS_v1,
		//MPMC_FS_LF_v1,

		maxQueueTypes
	};

	std::unordered_map<QueueType, string> queueTypeToQueueName{
		{ QueueType::MPMC_U_v1_deque, "MPMC_U_v1_deque"},
		{ QueueType::MPMC_U_v1_list, "MPMC_U_v1_list" },
		{ QueueType::MPMC_U_v1_fwlist, "MPMC_U_v1_fwlist" },
		{ QueueType::MPMC_U_v2_list, "MPMC_U_v2_list" },
		{ QueueType::MPMC_U_v2_fwlist, "MPMC_U_v2_fwlist" },
		{ QueueType::MPMC_U_v2_myfwlist, "MPMC_U_v2_myfwlist" },
		{ QueueType::MPMC_U_LF_v1, "MPMC_U_LF_v1" },
		{ QueueType::MPMC_FS_v1, "MPMC_FS_v1" }
		//{ QueueType::MPMC_FS_LF_v1, "MPMC_FS_LF_v1"}
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
	constexpr const int subCol3 = 9;
	constexpr const int subCol4 = 5;
	constexpr const int firstColWidth = subCol1 + subCol2 + subCol3 + subCol4;
	constexpr const int colWidth = 20;

	//================ end of global variables ================



	template<typename T>
	void producerThreadFunction(T& queue, size_t numProdOperationsPerThread, int threadId)
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
			queue.push(std::move(n));
		}
	}

	template<>
	void producerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>& queue, size_t numProdOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numProdOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				size_t sleepTime = sleepTimesNanosVec[threadId * numProdOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}
			int n = i;
			queue.push(std::move(n), threadId);
		}
	}

	template<typename T>
	void consumerThreadFunction(T& queue, size_t numConsOperationsPerThread, int threadId)
	{
		for(int i = 0; i < numConsOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				//int sleepTime = dist(mt64) % 100;
				size_t sleepTime = sleepTimesNanosVec[threadId * numConsOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}
			int n;
			//long long timeout = std::numeric_limits<long long>::max();
			std::chrono::milliseconds timeoutMilisec{ 60 * 1000 }; // timeout = 1 min
			bool result = queue.pop(n, timeoutMilisec);
			if (!result)
			{
				int n = 100; //This is just for debugging purpose
			}
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}
	template<>
	void consumerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>& queue, size_t numConsOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numConsOperationsPerThread; ++i)
		{
			if (useSleep)
			{
				int sleepTime = sleepTimesNanosVec[threadId * numConsOperationsPerThread + i];
				this_thread::sleep_for(chrono::nanoseconds(sleepTime));
			}
			int n = queue.pop(threadId);
		}
	}

	struct separate_thousands : std::numpunct<char> {
		char_type do_thousands_sep() const override { return ','; }  // separate with commas
		string_type do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	template<typename Tqueue>
	void test_mpmcu_queue(QueueType queueType, Tqueue& queue, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, int resultIndex)
	{
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

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
			if(i < numProducerThreads)
				producerThreads.push_back(std::thread(producerThreadFunction<Tqueue>, std::ref(queue), numProdOperationsPerThread, ++prodThreadId));
			if (i < numConsumerThreads)
				consumerThreads.push_back(std::thread(consumerThreadFunction<Tqueue>, std::ref(queue), numConsOperationsPerThread, ++consThreadId));
		}

		for (size_t i = 0; i < numProducerThreads; ++i)
			producerThreads[i].join();

		for (size_t i = 0; i < numConsumerThreads; ++i)
			consumerThreads[i].join();

		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
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

	template<typename Tqueue>
	void test_mpmcu_queue_sfinae(QueueType queueType, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, size_t queueSize, int resultIndex)
	{
		Tqueue queue{};
		test_mpmcu_queue(queueType, queue, numProducerThreads, numConsumerThreads, numOperations, resultIndex);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(QueueType queueType, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, size_t queueSize, int resultIndex)
	{
		MultiProducersMultiConsumersFixedSizeQueue_v1<int> queue{ queueSize };
		test_mpmcu_queue(queueType, queue, numProducerThreads, numConsumerThreads, numOperations, resultIndex);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(QueueType queueType, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperations, size_t queueSize, int resultIndex)
	{
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int> queue{ queueSize, numProducerThreads, numConsumerThreads };
		test_mpmcu_queue(queueType, queue, numProducerThreads, numConsumerThreads, numOperations, resultIndex);
	}
	
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
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int>>(QueueType::MPMC_U_v1_deque, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::list>>(QueueType::MPMC_U_v1_list, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::forward_list>>(QueueType::MPMC_U_v1_fwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v2<int, std::list>>(QueueType::MPMC_U_v2_list, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v2<int, std::forward_list>>(QueueType::MPMC_U_v2_fwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v2<int, Undefined>>(QueueType::MPMC_U_v2_myfwlist, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1<int>>(QueueType::MPMC_U_LF_v1, numProducerThreads, numConsumerThreads, numOperations, 0, resultIndex);

		/***** Fixed Size Queues ****/
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(QueueType::MPMC_FS_v1, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
		//The below queue does not work
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(QueueType::MPMC_FS_LF_v1, numProducerThreads, numConsumerThreads, numOperations, queueSize, resultIndex);
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

		int numOperations = 84000;
		int resultIndex = -1;

		cout << "\n=============== with sleep time ===============";
		useSleep = true;
		int divFactor = 40;
		std::vector<TestCase> testsWithSleep =
			{
				{ 1, 1,     numOperations / divFactor, 10 },
				{ 2, 2,     numOperations / divFactor, 10 },
				{ 3, 3,     numOperations / divFactor, 10 },
				{ 4, 4,     numOperations / divFactor, 10 },
				{ 5, 5,     numOperations / divFactor, 10 },
				{ 6, 6,     numOperations / divFactor, 10 },
				{ 7, 7,     numOperations / divFactor, 10 },
				{ 8, 8,     numOperations / divFactor, 10 },
				{ 9, 9,     numOperations / divFactor, 10 },
				{ 10, 10,   numOperations / divFactor, 10 },
				{ 20, 20,   numOperations / divFactor, 10 },
				{ 50, 50,   numOperations / divFactor, 10 },
				{ 100, 100, numOperations / divFactor, 10 },
			};

		results.insert(results.end(), testsWithSleep.begin(), testsWithSleep.end());
		
		for (int i = 0; i < testsWithSleep.size(); ++i)
		{
			cout << "\n"
				<< std::setw(subCol1) << testsWithSleep[i].numProducers_
				<< std::setw(subCol2) << testsWithSleep[i].numConsumers_
				<< std::setw(subCol3) << testsWithSleep[i].numOperations_
				<< std::setw(subCol4) << testsWithSleep[i].queueSize_;
			
			runTestCase(testsWithSleep[i].numProducers_, testsWithSleep[i].numConsumers_, testsWithSleep[i].numOperations_, testsWithSleep[i].queueSize_, ++resultIndex);
		}

		cout << "\n=============== without sleep time ===============";
		useSleep = false;
		std::vector<TestCase> testsWithoutSleep =
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
				{ 20, 20,   numOperations, 10 },
				{ 50, 50,   numOperations, 10 },
				{ 100, 100, numOperations, 10 },
			};

		results.insert(results.end(), testsWithoutSleep.begin(), testsWithoutSleep.end());
		
		for (int i = 0; i < testsWithoutSleep.size(); ++i)
		{
			cout << "\n"
				<< std::setw(subCol1) << testsWithoutSleep[i].numProducers_
				<< std::setw(subCol2) << testsWithoutSleep[i].numConsumers_
				<< std::setw(subCol3) << testsWithoutSleep[i].numOperations_
				<< std::setw(subCol4) << testsWithoutSleep[i].queueSize_;

			runTestCase(testsWithoutSleep[i].numProducers_, testsWithoutSleep[i].numConsumers_, testsWithoutSleep[i].numOperations_, testsWithoutSleep[i].queueSize_, ++resultIndex);
		}

	}
}

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


*/