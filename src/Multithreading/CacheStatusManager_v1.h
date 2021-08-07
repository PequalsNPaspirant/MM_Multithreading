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
#include <condition_variable>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"

namespace mm {

	namespace cacheStatusManager_v1 {

		template<typename T> struct is_shared_ptr : std::false_type {};
		template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
		template<typename T> struct is_shared_ptr<std::shared_ptr<const T>> : std::true_type {};

		class Hashable
		{
		public:
			virtual size_t hash() const = 0;
		};

		template<typename KeyType>
		class Hash
		{
		public:
			size_t operator()(const KeyType& key) const
			{
				return key.hash();
			}
		};

		std::string getThreadId()
		{
			std::thread::id id = std::this_thread::get_id();
			std::stringstream ss;
			ss << id;
			return ss.str();
		}

		// base case
		void log(std::ostream& out) {}

		template <typename T, typename... Args>
		void log(std::ostream& out, T&& t, Args&&... args)
		{
			out << std::forward<T>(t);
			log(out, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void log(const std::string& level, Args&&... args)
		{
			std::stringstream ss;
			ss << "\n" << level << " [" << getThreadId() << "] ";
			//using expander = int[];
			//(void)expander {
			//	0, (void(ss << ',' << std::forward<Args>(args)), 0)...
			//};
			log(ss, std::forward<Args>(args)...);

			std::cout << ss.str();
		}

		template<
			typename KeyType, typename ValueType,
			typename Hasher = std::hash<KeyType>, typename KeyEqual = std::equal_to<KeyType>,
			typename std::enable_if<is_shared_ptr<ValueType>::value>::type* = nullptr>
		class Cache
		{
		public:
			void set(const KeyType& key, ValueType value)
			{
				log("INFO", "Cache: Setting data in cache: key: ", key, " value: ", value);
				std::unique_lock<std::mutex> lock{ mu_ };
				cache_[key] = value;
			}

			ValueType get(const KeyType& key)
			{
				log("INFO", "Cache: Getting data from cache: key: ", key);
				std::unique_lock<std::mutex> lock{ mu_ };
				auto it = cache_.find(key);
				if (it != cache_.end())
					return it->second;

				return nullptr;
			}

			void clear()
			{
				log("INFO", "Cache: Clearing cache");
				std::unique_lock<std::mutex> lock{ mu_ };
				cache_.clear();
			}

		private:
			std::unordered_map<KeyType, ValueType, Hasher, KeyEqual> cache_;
			std::mutex mu_;
		};

		template<
			//template<typename... Args> typename CacheType,
			typename CacheType,
			typename KeyType, typename ValueType,
			typename Hasher = std::hash<KeyType>, typename KeyEqual = std::equal_to<KeyType>,
			typename std::enable_if<is_shared_ptr<ValueType>::value>::type* = nullptr>
		class CacheStatusManager
		{
		public:
			CacheStatusManager(CacheType& cache) : cache_{ cache } {}
			~CacheStatusManager() = default;

			template<typename Fun, typename... Args>
			ValueType setAndGet(const KeyType& key, int numRetries, bool wait, size_t timeoutMilliSec, Fun funToCreateValue, Args... args)
			{
				ValueType value;
				for (int tries = 0; !value && tries <= numRetries; ++tries) //if this thread timed out, try again for numRetries times
				{
					log("DEBG", "CacheStatusManager: setOrGet: tries: ", tries);
					WriteAccess wa = getWriteAccess(key);
					if (wa.accessGranted())
					{
						value = funToCreateValue(std::forward<Args>(args)...); //this function may throw
						wa.set(value);
					}
					else //wait for cache to be updated
						value = get(key, wait, timeoutMilliSec);
				}

				return value;
			}

			ValueType get(const KeyType& key, bool wait, size_t timeoutMilliSec)
			{
				std::unique_lock<std::mutex> lock{ muStatus_ };
				//No other thread deletes the entry from cache, so we can rely on reference to status without searching again and again
				CacheStatus& status = cacheStatus_[key];

				std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
				while (status == CacheStatus::preparing)
				{
					if (!wait) return nullptr;

					log("INFO", "CacheStatusManager: get: waiting: key: ", key);
					std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
					std::chrono::milliseconds elaspedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
					std::chrono::milliseconds remainingTimeout = std::chrono::milliseconds{ timeoutMilliSec } -elaspedTime;
					if (remainingTimeout.count() <= 0 || cvStatus_.wait_for(lock, remainingTimeout) == std::cv_status::timeout)
					{
						//The thread which was preparing data could not finish within time limit, reason may be exception, infinite loop etc.
						log("CRIT", "CacheStatusManager: get: timed out: key: ", key);
						if (status != CacheStatus::available) //If no other thread has yet pushed data in cache
						{
							status = CacheStatus::unavailable;
							lock.unlock();
							cvStatus_.notify_all();
							return nullptr;
						}

					}
				}

				if (status == CacheStatus::available)
					return cache_.get(key);

				return nullptr;
			}

			void clear()
			{
				log("INFO", "CacheStatusManager: clear: ");
				std::unique_lock<std::mutex> lock{ muStatus_ };
				cacheStatus_.clear();
				lock.unlock();

				cache_.clear();

				cvStatus_.notify_all(); //in case any thread is still waiting on cv. (THIS SHOULD NOT HAPPEN)
			}

		private:
			//No syncronization is required for this class
			class WriteAccess
			{
			public:
				WriteAccess(CacheStatusManager& cacheStatusMgr, const KeyType& key, bool accessGranted) :
					cacheStatusMgr_{ cacheStatusMgr },
					key_{ key },
					accessGranted_{ accessGranted }
				{}

				//Do not allow copy
				WriteAccess(const WriteAccess&) = delete;
				WriteAccess& operator=(const WriteAccess&) = delete;
				//Allow move
				WriteAccess(WriteAccess&&) = default;
				WriteAccess& operator=(WriteAccess&&) = default;

				~WriteAccess()
				{
					if (accessGranted_)
						cacheStatusMgr_.resetStatusIfStillPreparing(key_);
				}

				void set(ValueType value)
				{
					//if (!accessGranted_)
					//{
					//	//This may happen when the data preparing thread took long time than timeout, so another thread timedout and it changed status
					//  //Update: THIS SHOULD NOT HAPPEN because unless thread has accessGranted(), it can not call set() 
					//	log("CRIT", "[THIS SHOULD NOT HAPPEN] WriteAccess: set: forcefully setting data in cache: key: ", key_, " value: ", value);
					//	throw std::runtime_error{ "Trying to set data into cache without write access" };
					//}

					cacheStatusMgr_.set(key_, value);
				}

				bool accessGranted() { return accessGranted_; }

			private:
				CacheStatusManager& cacheStatusMgr_;
				KeyType key_;
				bool accessGranted_{ false };
			};

			WriteAccess getWriteAccess(const KeyType& key)
			{
				std::unique_lock<std::mutex> lock{ muStatus_ };

				CacheStatus& status = cacheStatus_[key];
				if (status == CacheStatus::unavailable)
				{
					log("INFO", "CacheStatusManager: getWriteAccess: access granted: key: ", key);
					status = CacheStatus::preparing;
					return WriteAccess{ *this, key, true };
				}
				else
					return WriteAccess{ *this, key, false };
			}

			void set(const KeyType& key, ValueType value)
			{
				std::unique_lock<std::mutex> lock{ muStatus_ };
				CacheStatus& status = cacheStatus_[key];
				status = CacheStatus::available;
				cache_.set(key, value); //update the cache under cacheStatus lock
				lock.unlock();

				cvStatus_.notify_all();
			}

			void resetStatusIfStillPreparing(const KeyType& key)
			{
				std::unique_lock<std::mutex> lock{ muStatus_ };
				CacheStatus& status = cacheStatus_[key];
				if (status == CacheStatus::preparing)
				{
					//If the status is still not 'available', probably due to exception or infinite loop etc in data preparing function,
					//and status may be still 'preparing', so reset the status and notify all threads
					//If the status is not 'preparing', some other thread might have timed out and changed status
					log("CRIT", "CacheStatusManager: resetStatusIfStillPreparing: data is still not cached");

					status = CacheStatus::unavailable;
					lock.unlock();
					cvStatus_.notify_all();
				}
			}

			enum class CacheStatus
			{
				unavailable = 0,
				preparing,
				available
			};

			CacheType& cache_;
			std::unordered_map<KeyType, CacheStatus, Hasher, KeyEqual> cacheStatus_;
			std::mutex muStatus_;
			std::condition_variable cvStatus_;
		};

	}

}

