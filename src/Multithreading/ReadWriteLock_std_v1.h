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

	namespace readWriteLock_stdMutex_v1 {

		class ReadWriteLock
		{
		public:
			void acquireReadLock()
			{
				//std::unique_lock<std::mutex> lock{ m_ };
				m_.lock();
			}

			void releaseReadLock()
			{
				m_.unlock();
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
			std::mutex m_;
		};

	}

	namespace readWriteLock_stdSharedMutex_v1 {

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
			std::shared_mutex m_;
		};

	}

}

