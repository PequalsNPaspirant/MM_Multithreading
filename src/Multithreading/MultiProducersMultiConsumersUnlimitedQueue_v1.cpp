#include <iostream>
#include <random>
using namespace std;

#include "MultiProducersMultiConsumersUnlimitedQueue_v1.h"
#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	std::random_device rd;
	std::mt19937 mt32(rd()); //for 32 bit system
	std::mt19937_64 mt64(rd()); //for 64 bit system
	std::uniform_int_distribution<int> dist(111, 999);

	template<typename T>
	void producerThreadFunction(T& queue)
	{
		for (int i = 0; i < 10; ++i)
		{
			int sleepTime = dist(mt64) % 100;
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = dist(mt64);
			//cout << "\nThread " << this_thread::get_id() << " pushing " << n << " into queue";
			queue.push(std::move(n));
		}
	}

	template<typename T>
	void consumerThreadFunction(T& queue)
	{
		for(int i = 0; i < 10; ++i)
		{
			int sleepTime = rand() % 100;
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = queue.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}

	template<typename T>
	void test_mpmcu_queue()
	{
		cout << "\n\nTest starts:";

		T queue;
		const int threadsCount = 10;
		vector<std::thread> producerThreads;
		vector<std::thread> consumerThreads;
		for (int i = 0; i < threadsCount; ++i)
		{
			producerThreads.push_back(std::thread(producerThreadFunction<T>, std::ref(queue)));
			consumerThreads.push_back(std::thread(consumerThreadFunction<T>, std::ref(queue)));
		}

		for (int i = 0; i < threadsCount; ++i)
		{
			producerThreads[i].join();
			consumerThreads[i].join();
		}

		cout << "\nfinished waiting for all threads. queue.size() = " << queue.size();
	}

	template <typename T>
	class Queue_v1
	{
	public:

		void push(T&& obj)
		{
			queue_.push(std::move(obj));
			cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
		}

		//exception UNSAFE pop() version
		T pop()
		{
			auto obj = queue_.front();
			queue_.pop();
			cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return obj; //this object can be returned by copy/move and it will be lost if the copy/move constructor throws exception.
		}

		//exception SAFE pop() version
		void pop(T& outVal)
		{
			outVal = queue_.front();
			queue_.pop();
			cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
		}

		size_t size()
		{
			return queue_.size();
		}

	private:
		std::queue<T> queue_;
	};

	MM_DECLARE_FLAG(Multithreading_mpmcu_queue);
	MM_UNIT_TEST(Multithreading_mpmcu_queue_test, Multithreading_mpmcu_queue)
	{
		MM_SET_PAUSE_ON_ERROR(true);

		test_mpmcu_queue<MultiProducerMultiConsumersUnlimitedQueue_v1<int>>();
		//test_mpmcu_queue<Queue_v1<int>>(); //This crashed the program due to lack of synchronization
	}
}