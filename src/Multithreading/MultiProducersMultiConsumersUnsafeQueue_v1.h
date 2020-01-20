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
	class UnsafeQueue_v1
	{
	public:

		void push(T&& obj)
		{
			queue_.push(std::move(obj));
			//cout << "\nThread " << this_thread::get_id() << " pushed " << obj << " into queue. Queue size: " << queue_.size();
		}

		//exception UNSAFE pop() version
		T pop()
		{
			auto obj = queue_.front();
			queue_.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
			return obj; //this object can be returned by copy/move and it will be lost if the copy/move constructor throws exception.
		}

		//exception SAFE pop() version
		void pop(T& outVal)
		{
			outVal = queue_.front();
			queue_.pop();
			//cout << "\nThread " << this_thread::get_id() << " popped " << obj << " from queue. Queue size: " << queue_.size();
		}

		size_t size()
		{
			return queue_.size();
		}

	private:
		std::queue<T> queue_;
	};
}