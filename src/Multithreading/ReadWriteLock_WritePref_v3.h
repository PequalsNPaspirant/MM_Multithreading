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

	namespace readWriteLock_WritePref_v3 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				//while (numWritersWaiting_ > 0 || writterActive_)
				while (numWritersActive_ > 0)
					cv_.wait(lock);

				++numReadersActive_;

				lock.unlock();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				--numReadersActive_;
				lock.unlock();

				if (numReadersActive_ == 0)
					cv_.notify_all();
			}

			void lock()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				++numWritersActive_;

				constexpr const int numParallelWritersAllowed = 1;
				//while (numReadersActive_ > 0 || writterActive_)
				while (numReadersActive_ > 0 || numWritersActive_ > numParallelWritersAllowed)
					cv_.wait(lock);

				//writterActive_ = true;
				lock.unlock();
			}

			void unlock()
			{
				std::unique_lock<std::mutex> lock{ mu_ };
				--numWritersActive_;
				//writterActive_ = false;
				lock.unlock();

				cv_.notify_all();
			}

		private:
			std::mutex mu_;
			std::condition_variable cv_;

			int numReadersActive_{ 0 };
			int numWritersActive_{ 0 };
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
