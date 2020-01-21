#include <iostream>
#include <random>
#include <chrono>
#include <locale>
#include <memory>
using namespace std;

#include "MultiProducersMultiConsumersUnsafeQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v1.h"
#include "MultiProducersMultiConsumersFixedSizeQueue_v1.h"
#include "MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	std::random_device rd;
	std::mt19937 mt32(rd()); //for 32 bit system
	std::mt19937_64 mt64(rd()); //for 64 bit system
	std::uniform_int_distribution<int> dist(111, 999);
	vector<int> sleepTimesVec;

	template<typename T>
	void producerThreadFunction(T& queue, int numOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
			//int sleepTime = dist(mt64) % 100;
			int sleepTime = sleepTimesVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			//int n = dist(mt64);
			//cout << "\nThread " << this_thread::get_id() << " pushing " << n << " into queue";
			int n = i;
			queue.push(std::move(n));
		}
	}

	template<>
	void producerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>& queue, int numOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
			int sleepTime = sleepTimesVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));
			int n = i;
			queue.push(std::move(n), threadId);
		}
	}

	template<typename T>
	void consumerThreadFunction(T& queue, int numOperationsPerThread, int threadId)
	{
		for(int i = 0; i < numOperationsPerThread; ++i)
		{
			//int sleepTime = dist(mt64) % 100;
			int sleepTime = sleepTimesVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = queue.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}
	template<>
	void consumerThreadFunction<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>>(MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>& queue, int numOperationsPerThread, int threadId)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
			int sleepTime = sleepTimesVec[threadId * numOperationsPerThread + i];
			this_thread::sleep_for(chrono::milliseconds(sleepTime));
			int n = queue.pop(threadId);
		}
	}

	struct separate_thousands : std::numpunct<char> {
		char_type do_thousands_sep() const override { return ','; }  // separate with commas
		string_type do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	template<typename Tqueue>
	void test_mpmcu_queue(Tqueue& queue, size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread)
	{
		cout << "\n\nTest starts:";

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
		cout << "\nfinished waiting for all threads. Duration: " << duration << " nanoseconds. Queue.size() = " << queue.size();
	}

	template<typename Tqueue>
	void test_mpmcu_queue_sfinae(size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, size_t queueSize)
	{
		Tqueue queue{};
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, size_t queueSize)
	{
		MultiProducersMultiConsumersFixedSizeQueue_v1<int> queue{ queueSize };
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>>(size_t numProducerThreads, size_t numConsumerThreads, size_t numOperationsPerThread, size_t queueSize)
	{
		MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int> queue{ queueSize, numProducerThreads, numConsumerThreads };
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread);
	}

	MM_DECLARE_FLAG(Multithreading_mpmcu_queue);
	MM_UNIT_TEST(Multithreading_mpmcu_queue_test, Multithreading_mpmcu_queue)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		size_t numProducerThreads = 25;
		size_t numConsumerThreads = 25;
		size_t numOperationsPerThread = 50;
		size_t queueSize = 8; // numProducerThreads * numOperationsPerThread / 10;

		size_t total = (numProducerThreads + numConsumerThreads) * numOperationsPerThread;
		sleepTimesVec.reserve(total);
		for (size_t i = 0; i < total; ++i)
		{
			int sleepTime = dist(mt64) % 100;
			sleepTimesVec.push_back(sleepTime);
		}

		//cout << "\n\n============================ Testing Unsafe Queue (v1)...";
		//test_mpmcu_queue_sfinae<UnsafeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0); //This crashes the program due to lack of synchronization
		cout << "\n\n============================ Testing Multi Producers Multi Consumers Unlimited Queue (v1)...";
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0);
		cout << "\n\n============================ Testing Multi Producers Multi Consumers Fixed Size Queue (v1)...";
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, queueSize);
		//cout << "\n\n============================ Testing Multi Producers Multi Consumers Fixed Size Lock Free Queue (v1)...";
		//test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeLockFreeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, queueSize);
	}
}