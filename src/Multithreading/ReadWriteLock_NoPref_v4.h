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

	namespace readWriteLock_NoPref_v4 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				++numReadersWaiting_;

				//Assuming cv honours the sequence in which the threads are waiting on queue, if writer was woken up, do not allow readers because some writer was waiting in sequence on cv
				//while (writterActive_ || writterWokenUp_)
				while (numWritersActive_ > 0 || writterWokenUp_)
					cv_.wait(lock);

				--numReadersWaiting_;
				++numReadersActive_;

				lock.unlock();
			}

			void unlock_shared()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				--numReadersActive_;

				if (numReadersActive_ == 0)
				{
					lock.unlock();
					cv_.notify_all();
				}
			}

			void lock()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				++numWritersWaiting_;

				//while (numReadersActive_ > 0 || writterActive_)
				while (numReadersActive_ > 0 || numWritersActive_ >= numConcurrentWritersAllowed)
				{
					cv_.wait(lock);
					writterWokenUp_ = true;
				}

				writterWokenUp_ = false;

				--numWritersWaiting_;
				++numWritersActive_;
				//writterActive_ = true;

				lock.unlock();
			}

			void unlock()
			{
				std::unique_lock<std::mutex> lock{ mu_ };

				--numWritersActive_;
				//writterActive_ = false;

				if (numReadersWaiting_ > 0)
				{
					lock.unlock();
					cv_.notify_all(); //notify all readers
				}
				else
				{
					lock.unlock();
					cv_.notify_one(); //notify next one writer waiting on cv
				}
			}

		private:
			std::mutex mu_;
			std::condition_variable cv_;

			int numReadersWaiting_{ 0 };
			int numReadersActive_{ 0 };
			int numWritersWaiting_{ 0 };
			int numWritersActive_{ 0 };
			//bool writterActive_{ false };
			static constexpr const int numConcurrentWritersAllowed{ 1 };

			bool writterWokenUp_{ false };
			/*
			writterWokenUp_ is important to honour the order of read/write operations tried by user.
			1. A writer thread w1 takes write lock and starts writing to data.
			2. A reader thread r1 comes and keeps waiting for writing task to finish.
			3. A writer thread w2 comes and keeps waiting.
			4. A reader thread r2 comes and keeps waiting.
			5. w1 finishes writing and then calls cv_.notify_all()
			6. r1 wakes up and starts reading
			7. w2 wakes up and keeps waiting again. BUT IT SETS writterWokenUp_ = true.
			8. r2 wakes up, but writterWokenUp_ is true, so it keeps waiting, Otherwise it will also join r1 and start reading.
			*/
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

