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
#include <atomic>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	//This implementation works but it is very slow. It takes ~10 min while std::shared_mutex takes ~2 sec
	namespace readWriteLock_NoPref_LockFree_v4_1 {
		class SharedMutex
		{
		public:
			void lock_shared()
			{
				size_t uniqueId = uniqueId_.fetch_add(1);
				while (uniqueId > uniqueIdCurrent_.load(std::memory_order_acquire))
					std::this_thread::yield();

				++numReadersActive_;
				++uniqueIdCurrent_;
			}

			void unlock_shared()
			{
				--numReadersActive_;
			}

			void lock()
			{
				size_t uniqueId = uniqueId_.fetch_add(1);
				while (numReadersActive_.load(std::memory_order_acquire) > 0 || uniqueId > uniqueIdCurrent_.load(std::memory_order_acquire))
					std::this_thread::yield();
			}

			void unlock()
			{
				++uniqueIdCurrent_;
			}

		private:
			std::atomic<size_t> uniqueId_{ 0 };
			std::atomic<size_t> uniqueIdCurrent_{ 0 };
			std::atomic<int> numReadersActive_{ 0 };
		};

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

	//This implementation works but it is very slow. It takes ??????? min while std::shared_mutex takes ~2 sec
	namespace readWriteLock_NoPref_LockFree_v4_2 {
		class SharedMutex
		{
		public:
			void lock_shared()
			{
				size_t uniqueId = uniqueId_.fetch_add(1);
				while (uniqueId > uniqueIdCurrent_.load(std::memory_order_acquire))
					std::this_thread::yield();

				++uniqueIdCurrent_;
			}

			void unlock_shared()
			{
				++numReadersWritersDone_;
			}

			void lock()
			{
				size_t uniqueId = uniqueId_.fetch_add(1);
				while ((uniqueId - 1) > numReadersWritersDone_.load(std::memory_order_acquire))
					std::this_thread::yield();
			}

			void unlock()
			{
				++uniqueIdCurrent_;
				++numReadersWritersDone_;
			}

		private:
			std::atomic<size_t> uniqueId_{ 0 };
			std::atomic<size_t> uniqueIdCurrent_{ 0 };
			std::atomic<size_t> numReadersWritersDone_{ 0 };
		};

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

	//honours the reader's id, but not the writer's id
	namespace readWriteLock_NoPref_LockFree_v4_3 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				size_t uniqueId = uniqueId_.fetch_add(1);
				//++uniqueIdCurrent_;

				//while (uniqueId > uniqueIdCurrent_.load(std::memory_order_acquire))
				while (uniqueId > nextWriterId_.load(std::memory_order_acquire))
					std::this_thread::yield();

				++numReadersActive_;
			}

			void unlock_shared()
			{
				--numReadersActive_;

				++numReadersWritersDone_;
			}

			void lock()
			{
				while(writterActive_.exchange(true))
					std::this_thread::yield();

				size_t uniqueId = uniqueId_.fetch_add(1);
				nextWriterId_.store(uniqueId, std::memory_order_release);

				while ((uniqueId - 1) > numReadersWritersDone_.load(std::memory_order_acquire)) // || writterActive_.exchange(true))
					std::this_thread::yield();

				++numWritersActive_;
			}

			void unlock()
			{
				--numWritersActive_;

				++numReadersWritersDone_;
				//++uniqueIdCurrent_;
				writterActive_.store(false, std::memory_order_release);
			}

		private:
			std::atomic<size_t> uniqueId_{ 1 };
			//std::atomic<size_t> uniqueIdCurrent_{ 0 };
			std::atomic<size_t> numReadersWritersDone_{ 0 };
			std::atomic<size_t> nextWriterId_{ std::numeric_limits<size_t>::max() };


			std::atomic<bool> writterActive_{ false };
			std::atomic<int> numWritersActive_{ 0 };
			std::atomic<int> numReadersActive_{ 0 };
		};

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

	//Does not honour the reader's and writer's id, but honours the number of readers and writers that come in subsequent groups
	namespace readWriteLock_NoPref_LockFree_v4_4 {

		class SharedMutex
		{
		public:
			void lock_shared()
			{
				size_t uniqueId = uniqueId_.fetch_add(1);
				size_t numReadersWritersDone = numReadersWritersDone_.load(std::memory_order_acquire);
				while ((numReadersWritersDone - 1) >= nextWriterId_.load(std::memory_order_acquire)
					|| !numReadersWritersDone_.compare_exchange_weak(numReadersWritersDone, numReadersWritersDone + 1))
				{
					std::this_thread::yield();
				}

				++numReadersActive_;
			}

			void unlock_shared()
			{
				--numReadersActive_;
			}

			void lock()
			{
				while (writterActive_.exchange(true))
					std::this_thread::yield();

				size_t uniqueId = uniqueId_.fetch_add(1);
				nextWriterId_.store(uniqueId, std::memory_order_release);

				while ((uniqueId - 1) > numReadersWritersDone_.load(std::memory_order_acquire)) // || writterActive_.exchange(true))
					std::this_thread::yield();

				++numWritersActive_;
			}

			void unlock()
			{
				--numWritersActive_;

				++numReadersWritersDone_;
				writterActive_.store(false, std::memory_order_release);
			}

		private:
			std::atomic<size_t> uniqueId_{ 1 };
			std::atomic<size_t> numReadersWritersDone_{ 0 };
			std::atomic<bool> writterActive_{ false };
			std::atomic<size_t> nextWriterId_{ std::numeric_limits<size_t>::max() };

			//std::atomic<size_t> uniqueIdCurrent_{ 0 };
			std::atomic<int> numWritersActive_{ 0 };
			std::atomic<int> numReadersActive_{ 0 };
		};

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

