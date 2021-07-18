//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	namespace readWriteLock_NoPref_v2 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				while (numWriters_ > 0)
					cv_.wait(lock);

				++numReaders_;

				lock.unlock();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				--numReaders_;
				lock.unlock();

				cv_.notify_all();
			}

			void lock()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				//while (numReaders_ > 0 || writterActive_)
				while (numReaders_ > 0 || numWriters_ > 0)
					cv_.wait(lock);

				//writterActive_ = true;
				++numWriters_;
				lock.unlock();
			}

			void unlock()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				--numWriters_;
				//writterActive_ = false;
				lock.unlock();

				cv_.notify_all();
			}

		private:
			std::mutex mu_;
			std::condition_variable cv_;

			int numReaders_{ 0 };
			int numWriters_{ 0 };
			//bool writterActive_{ false };
		};

		/*
		All Cases:
		0 -                                                                        start


		1 -                                   R1                                                                          W1 


		2 -               R2                                  W2                       	              R2                                  W2             

		3 -       R3               W3                 R3               W3                     R3               W3                 R3               W3     
		4 -    R4    W4         R4    W4		   R4    W4         R4    W4			   R4    W4         R4    W4		   R4    W4         R4    W4
		*/

		class ReadWriteLock
		{
		public:
			void acquireReadLock()
			{
				m_.lock_shared();
			}

			void releaseReadLock()
			{
				m_.unlock_shared();
			}

			void acquireWriteLock()
			{
				m_.lock();
			}

			void releaseWriteLock()
			{
				m_.unlock();
			}

		private:
			SharedMutex m_;
		};

	}

}

