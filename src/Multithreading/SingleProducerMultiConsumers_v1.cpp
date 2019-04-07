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
	class spmc_fifo_queue
	{
	public:

		void push(T&& obj)
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			queue_.push(std::move(obj));
			cout << "\nThread " << this_thread::get_id() << " pushing " << obj << " into queue. Queue size: " << queue_.size();
			mlock.unlock();
			cond_.notify_one();
		}

		T pop()
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			while (queue_.empty())
			{
				cond_.wait(mlock);
			}
			auto obj = queue_.front();
			queue_.pop();
			cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return obj;
		}

	private:
		std::queue<T> queue_;
		std::mutex mutex_;
		std::condition_variable cond_;
	};

	void producerThreadFunction(spmc_fifo_queue<int>& queue)
	{
		for (int i = 0; i < 10; ++i)
		{
			int sleepTime = rand() % 100;
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = rand() % 100;
			//cout << "\nThread " << this_thread::get_id() << " pushing " << n << " into queue";
			queue.push(std::move(n));
		}
	}

	void consumerThreadFunction(spmc_fifo_queue<int>& queue)
	{
		for(int i = 0; i < 10; ++i)
		{
			int sleepTime = rand() % 100;
			this_thread::sleep_for(chrono::milliseconds(sleepTime));

			int n = queue.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << n << " from queue";
		}
	}

	void test_spmc_queue()
	{
		//This queue supports multiple producers
		spmc_fifo_queue<int> queue;

		const int threadsCount = 10;
		vector<std::thread> producerThreads;
		vector<std::thread> consumerThreads;
		for (int i = 0; i < threadsCount; ++i)
		{
			producerThreads.push_back(std::thread(producerThreadFunction, std::ref(queue)));
			consumerThreads.push_back(std::thread(consumerThreadFunction, std::ref(queue)));
		}

		for (int i = 0; i < threadsCount; ++i)
		{
			producerThreads[i].join();
			consumerThreads[i].join();
		}

		cout << "\nfinished waiting for all therads";
	}

	MM_DECLARE_FLAG(Multithreading_spmc_fifo_queue);
	MM_UNIT_TEST(Multithreading_spmc_fifo_queue_test, Multithreading_spmc_fifo_queue)
	{
		//MM_SET_PAUSE_ON_ERROR(true);

		test_spmc_queue();
	}
}