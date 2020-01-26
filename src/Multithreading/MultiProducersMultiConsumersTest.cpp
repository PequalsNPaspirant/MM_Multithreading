#include <iostream>
#include <random>
#include <chrono>
#include <locale>
#include <memory>
#include <vector>
#include <list>
#include <forward_list>
using namespace std;

#include "MultiProducersMultiConsumersUnsafeQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedLockFreeQueue_v2.h"

#include "MultiProducersMultiConsumersFixedSizeQueue_v1.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx.h"

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

//#define USE_SLEEP

namespace mm {

	std::random_device rd;
	std::mt19937 mt32(rd()); //for 32 bit system
	std::mt19937_64 mt64(rd()); //for 64 bit system
	std::uniform_int_distribution<int> dist(111, 999);
	vector<int> sleepTimesMilliVec;
	size_t totalSleepTimeNanos;
	/*
	Results in the tabular format like below: (where (a,b,c) = a producers b consumers and c operations by each thread)
	(#producdrs, #consumers, #operations)          Queue1       Queue2
	(1,1,1)        
	(1,1,10)        
	(2,2,2)
	*/

	struct ResultSet
	{
		int numProducers_;
		int numConsumers_;
		int numOperations_;
		int queueSize_;
		std::vector<size_t> nanosPerQueueType_;
	};

	std::vector<ResultSet> results;

	template<typename T>
	void producerThreadFunction(T& queue, size_t numOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
#ifdef USE_SLEEP
			//int sleepTime = dist(mt64) % 100;
			int sleepTime = sleepTimesMilliVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));
#endif
			//int n = dist(mt64);
			//cout << "\nThread " << this_thread::get_id() << " pushing " << n << " into queue";
			int n = i;
			queue.push(std::move(n));
		}
	}

	template<>
	void producerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>& queue, size_t numOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
#ifdef USE_SLEEP
			int sleepTime = sleepTimesMilliVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));
#endif
			int n = i;
			queue.push(std::move(n), threadId);
		}
	}

	template<typename T>
	void consumerThreadFunction(T& queue, size_t numOperationsPerThread, int threadId)
	{
		for(int i = 0; i < numOperationsPerThread; ++i)
		{
#ifdef USE_SLEEP
			//int sleepTime = dist(mt64) % 100;
			int sleepTime = sleepTimesMilliVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));
#endif
			int n = queue.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}
	template<>
	void consumerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>& queue, size_t numOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
#ifdef USE_SLEEP
			int sleepTime = sleepTimesMilliVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));
#endif
			int n = queue.pop(threadId);
		}
	}

	struct separate_thousands : std::numpunct<char> {
		char_type do_thousands_sep() const override { return ','; }  // separate with commas
		string_type do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	template<typename Tqueue>
	void test_mpmcu_queue(Tqueue& queue, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, int resultIndex)
	{
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

		//Tqueue queue = createQueue_sfinae<Tqueue>(queueSize);
		const size_t threadsCount = numProducerThreads > numConsumerThreads ? numProducerThreads : numConsumerThreads;
		vector<std::thread> producerThreads;
		vector<std::thread> consumerThreads;
		int threadId = -1;
		for (size_t i = 0; i < threadsCount; ++i)
		{
			if(i < numProducerThreads)
				producerThreads.push_back(std::thread(producerThreadFunction<Tqueue>, std::ref(queue), numOperationsPerThread, ++threadId));
			if (i < numConsumerThreads)
				consumerThreads.push_back(std::thread(consumerThreadFunction<Tqueue>, std::ref(queue), numOperationsPerThread, ++threadId));
		}

		for (size_t i = 0; i < numProducerThreads; ++i)
			producerThreads[i].join();

		for (size_t i = 0; i < numConsumerThreads; ++i)
			consumerThreads[i].join();

		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
		unsigned long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

		//int number = 123'456'789;
		//std::cout << "\ndefault locale: " << number;
		auto thousands = std::make_unique<separate_thousands>();
		std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
		//std::cout << "\nlocale with modified thousands: " << number;
		cout << "\nfinished waiting for all threads. Total Duration: " << duration << " nanoseconds. Queue is empty? : " << (queue.empty() ? "Yes" : "No") << " Queue Size: " << queue.size();
		results[resultIndex].nanosPerQueueType_.push_back(duration);
	}

	template<typename Tqueue>
	void test_mpmcu_queue_sfinae(size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, size_t queueSize, int resultIndex)
	{
		Tqueue queue{};
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread, resultIndex);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, size_t queueSize, int resultIndex)
	{
		MultiProducersMultiConsumersFixedSizeQueue_v1<int> queue{ queueSize };
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread, resultIndex);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int>>(size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, size_t queueSize, int resultIndex)
	{
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_vx<int> queue{ queueSize, numProducerThreads, numConsumerThreads };
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread, resultIndex);
	}
	
	void runTestCase(int numProducerThreads, int numConsumerThreads, int numOperationsPerThread, int queueSize, int resultIndex)
	{
		//size_t  = 25;
		//size_t  = 25;
		//size_t  = 50;
		//size_t  = 8; // numProducerThreads * numOperationsPerThread / 10;

#ifdef USE_SLEEP
		size_t total = (numProducerThreads + numConsumerThreads) * numOperationsPerThread;
		sleepTimesMilliVec.reserve(total);
		totalSleepTimeNanos = 0;
		for (size_t i = 0; i < total; ++i)
		{
			int sleepTime = dist(mt64) % 100;
			totalSleepTimeNanos += sleepTime;
			sleepTimesMilliVec.push_back(sleepTime);
		}
		totalSleepTimeNanos *= 1000000ULL;
#endif

		/***** Unlimited Queues ****/
		//The below queue crashes the program due to lack of synchronization
		//test_mpmcu_queue_sfinae<UnsafeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex); 
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::vector<int>>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex);
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::vector>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::list>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int, std::forward_list>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex);
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedLockFreeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0, resultIndex);

		/***** Fixed Size Queues ****/
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, queueSize, resultIndex);
		//The below queue does not work
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, queueSize, resultIndex);
	}

	MM_DECLARE_FLAG(Multithreading_mpmcu_queue);
	MM_UNIT_TEST(Multithreading_mpmcu_queue_test, Multithreading_mpmcu_queue)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		results =
		{
		{ 1, 1, 1, 1, {} },
		{ 1, 1, 100, 10, {} },
		{ 2, 2, 100, 10, {} },
		{ 3, 3, 100, 10, {} },
		{ 4, 4, 100, 10, {} },
		{ 4, 4, 500, 10, {} },
		{ 5, 5, 100, 10, {} },
		{ 6, 6, 100, 10, {} },
		{ 7, 7, 100, 10, {} },
		{ 8, 8, 100, 10, {} },
		{ 9, 9, 100, 10, {} },
		{ 10, 10, 100, 10, {} },
		{ 10, 10, 500, 10, {} },
		{ 20, 20, 5000, 10,{} },
		{ 100, 100, 100000, 10,{} },
		};

		for (int i = 0; i < results.size(); ++i)
		{
			cout << "\nTest case no.: " << i;
			runTestCase(results[i].numProducers_, results[i].numConsumers_, results[i].numOperations_, results[i].queueSize_, i);
		}

		//Print results
		constexpr const int subCol1 = 5;
		constexpr const int subCol2 = 5;
		constexpr const int subCol3 = 9;
		constexpr const int subCol4 = 5;
		constexpr const int firstColWidth = subCol1 + subCol2 + subCol3 + subCol4;
		constexpr const int colWidth = 20;
		cout
			<< "\n\n"
			<< std::setw(firstColWidth) << "Test Case"
			<< std::setw(colWidth) << "MPMC-U-v1-deque"
			<< std::setw(colWidth) << "MPMC-U-v1-list"
			<< std::setw(colWidth) << "MPMC-U-v1-fwlist"
			<< std::setw(colWidth) << "MPMC-U-LF-v1"
			<< std::setw(colWidth) << "MPMC-FS-v1"
			<< std::setw(colWidth) << "MPMC-FS-LF-v1"
			<< "\n";
		cout << "\n"
			<< std::setw(5) << "Ps"
			<< std::setw(5) << "Cs"
			<< std::setw(6) << "Ops"
			<< std::setw(4) << "Qsz";
		for (int i = 0; i < results.size(); ++i)
		{
			cout << "\n"
				<< std::setw(subCol1) << results[i].numProducers_
				<< std::setw(subCol2) << results[i].numConsumers_
				<< std::setw(subCol3) << results[i].numOperations_
				<< std::setw(subCol4) << results[i].queueSize_;
			for (int k = 0; k < results[i].nanosPerQueueType_.size(); ++k)
				cout << std::setw(colWidth) << results[i].nanosPerQueueType_[k];
		}
	}
}