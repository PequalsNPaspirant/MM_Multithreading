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

	namespace readWriteLock_ReadPref_v1 {

		class SharedMutex
		{
		public:
			void lock()
			{

			}

			void unlock()
			{

			}

			void lock_shared()
			{

			}

			void unlock_shared()
			{

			}
		};

		class ReadWriteLock
		{
		public:
			void acquireReadLock()
			{
				//std::unique_lock<std::mutex> lock{ m_ };
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

