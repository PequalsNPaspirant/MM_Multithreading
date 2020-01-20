#include <iostream>
#include <random>
#include <chrono>
#include <locale>
#include <memory>
using namespace std;

#include "MultiProducersMultiConsumersUnsafeQueue_v1.h"
#include "MultiProducersMultiConsumersUnlimitedQueue_v1.h"
#include "MultiProducersMultiConsumersFixedSizeQueue_v1.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	std::random_device rd;
	std::mt19937 mt32(rd()); //for 32 bit system
	std::mt19937_64 mt64(rd()); //for 64 bit system
	std::uniform_int_distribution<int> dist(111, 999);

	template<typename T>
	void producerThreadFunction(T& queue, int numOperationsPerThread)
	{
		for (int i = 0; i < numOperationsPerThread; ++i)
		{
			int sleepTime = dist(mt64) % 100;
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = dist(mt64);
			//cout << "\nThread " << this_thread::get_id() << " pushing " << n << " into queue";
			queue.push(std::move(n));
		}
	}

	template<typename T>
	void consumerThreadFunction(T& queue, int numOperationsPerThread)
	{
		for(int i = 0; i < numOperationsPerThread; ++i)
		{
			int sleepTime = rand() % 100;
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = queue.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}

	struct separate_thousands : std::numpunct<char> {
		char_type do_thousands_sep() const override { return ','; }  // separate with commas
		string_type do_grouping() const override { return "\3"; } // groups of 3 digit
	};

	template<typename Tqueue>
	void test_mpmcu_queue(Tqueue& queue, int numProducerThreads, int numConsumerThreads, int numOperationsPerThread)
	{
		cout << "\n\nTest starts:";

		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

		//Tqueue queue = createQueue_sfinae<Tqueue>(queueSize);
		const int threadsCount = numProducerThreads > numConsumerThreads ? numProducerThreads : numConsumerThreads;
		vector<std::thread> producerThreads;
		vector<std::thread> consumerThreads;
		for (int i = 0; i < threadsCount; ++i)
		{
			if(i < numProducerThreads)
				producerThreads.push_back(std::thread(producerThreadFunction<Tqueue>, std::ref(queue), numOperationsPerThread));
			if (i < numConsumerThreads)
				consumerThreads.push_back(std::thread(consumerThreadFunction<Tqueue>, std::ref(queue), numOperationsPerThread));
		}

		for (int i = 0; i < numProducerThreads; ++i)
			producerThreads[i].join();

		for (int i = 0; i < numConsumerThreads; ++i)
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
	void test_mpmcu_queue_sfinae(int numProducerThreads, int numConsumerThreads, int numOperationsPerThread, size_t queueSize)
	{
		Tqueue queue{};
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread);
	}
	template<>
	void test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(int numProducerThreads, int numConsumerThreads, int numOperationsPerThread, size_t queueSize)
	{
		MultiProducersMultiConsumersFixedSizeQueue_v1<int> queue{ queueSize };
		test_mpmcu_queue(queue, numProducerThreads, numConsumerThreads, numOperationsPerThread);
	}

	MM_DECLARE_FLAG(Multithreading_mpmcu_queue);
	MM_UNIT_TEST(Multithreading_mpmcu_queue_test, Multithreading_mpmcu_queue)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		int numProducerThreads = 25;
		int numConsumerThreads = 25;
		int numOperationsPerThread = 50;
		size_t queueSize = 5; // numProducerThreads * numOperationsPerThread / 10;

		cout << "\n\n============================ Testing Unsafe Queue...";
		//test_mpmcu_queue_sfinae<UnsafeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0); //This crashes the program due to lack of synchronization
		cout << "\n\n============================ Testing Multi Producers Multi Consumers Unlimited Queue...";
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersUnlimitedQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, 0);
		cout << "\n\n============================ Testing Multi Producers Multi Consumers Fixed Size Queue...";
		test_mpmcu_queue_sfinae<MultiProducersMultiConsumersFixedSizeQueue_v1<int>>(numProducerThreads, numConsumerThreads, numOperationsPerThread, queueSize);
	}
}