//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <string>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

/*

NOTE: condition_variable can NOT be implemented using just mutexes. 
Basic reason: mutex has owenership. Mutex locked by one thread can not be unlocked by other thread. (https://en.cppreference.com/w/cpp/thread/mutex/unlock)

https://stackoverflow.com/questions/3710017/how-to-write-your-own-condition-variable-using-atomic-primitives

*/

namespace mm {

	namespace ConditionVariableUsingMutex {

		class ConditionVariableUsingMutex
		{
		public:
			ConditionVariableUsingMutex()
			{
				m_.lock(); //This thread has to also unlock this mutex because this thread is now owner of mutex 
			}

			void wait(std::unique_lock<std::mutex>& lock)
			{
				lock.unlock();
				//block this thread
				m_.lock();
			}

			void notify_one()
			{
				//release blocked thread
				m_.unlock(); //can NOT unlock mutex if current thread is not the owner, i.e. if current thread has not locked it
			}

			void notify_all()
			{
				//release all blocked threads
			}

		private:
			std::mutex m_;
			
		};

		class ThreadSafeQueue
		{
		public:
			void push(int n)
			{
				std::unique_lock<std::mutex> lock{ m_ };
				q_.push(n);
				++count_;
				lock.unlock();

				cv_.notify_one();
			}

			int pop()
			{
				std::unique_lock<std::mutex> lock{ m_ };

				while (count_ == 0)
					cv_.wait(lock);

				--count_;
				int retVal = q_.front();
				q_.pop();
				return retVal;
			}

		private:
			std::queue<int> q_;
			std::mutex m_;
			ConditionVariableUsingMutex cv_;
			int count_ = 0;
		};

		void usage()
		{

		}
	}

}

