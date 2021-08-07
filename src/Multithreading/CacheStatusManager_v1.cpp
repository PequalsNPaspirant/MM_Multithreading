//Goal
//Create a task container and executor identified by string/enum

#include <iostream>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include <thread>
#include <random>

#include "MM_UnitTestFramework/MM_UnitTestFramework.h"
#include "CacheStatusManager_v1.h"

namespace mm {

	namespace cacheStatusManager_v1 {

		struct MyKey : public Hashable
		{
			MyKey(int val) : val_{ val } {}
			MyKey(const MyKey&) = default;
			MyKey(MyKey&&) = default;
			MyKey& operator=(const MyKey&) = default;
			MyKey& operator=(MyKey&&) = default;

			size_t hash() const override
			{
				return std::hash<int>{}(val_);
			}

			bool operator==(const MyKey& rhs) const
			{
				return val_ == rhs.val_;
			}

			friend ostream& operator<<(ostream& o, const MyKey& key)
			{
				o << key.val_;
				return o;
			}

			int val_;
		};

		struct MyValue
		{
			MyValue(int val) : val_{ val } {}

			friend ostream& operator<<(ostream& o, const MyValue& value)
			{
				o << value.val_;
				return o;
			}

			friend ostream& operator<<(ostream& o, std::shared_ptr<MyValue> value)
			{
				o << value->val_;
				return o;
			}

			int val_;
		};

		void testCacheStatusManager(bool generateRandomKey)
		{
			bool ready = false;
			std::mutex mu;
			std::condition_variable cv;

			using CacheType = Cache<MyKey, std::shared_ptr<MyValue>, Hash<MyKey>>;
			using CacheStatusMgrType = CacheStatusManager<CacheType, MyKey, std::shared_ptr<MyValue>, Hash<MyKey>>;
			auto threadFunction = [&](CacheStatusMgrType& cacheStatusMgr, int numIterations, size_t timeoutMilliSec) {

				std::random_device rd;
				std::mt19937 mt(rd());
				std::uniform_int_distribution<int> distKey(1, numIterations);
				std::uniform_int_distribution<size_t> distTimeout(1, timeoutMilliSec / 2);
				std::uniform_int_distribution<int> distBehavior(1, 100);

				auto prepareDataToCache = [timeoutMilliSec](int val, size_t currentTimeout, int behavior) {

					switch (behavior % 10)
					{
					case 0:
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						//sleep for some time less than timeout. Probability = 8/10
						std::this_thread::sleep_for(std::chrono::milliseconds{ currentTimeout }); break;
					case 8:
						//randomly delay to make other threads time out. Probability = 1/10
						std::this_thread::sleep_for(std::chrono::milliseconds{ timeoutMilliSec + 50 }); break;
					case 9:
						//randomly throw. Probability = 1/10
						throw std::runtime_error{ "destructive testing: throwing intentionally" }; break;
					default:
						//very randomly go into infinite loop. Probability = 1/100
						if (behavior == 100)
						{
							log("CRIT", "------------------ Going into infinite loop: key: ", val);
							while (true)
								std::this_thread::sleep_for(std::chrono::milliseconds{ currentTimeout });
						}
					}

					return std::make_shared<MyValue>(val);
				};

				int numRetries = 3;
				bool wait = true;

				//Wait for all threads to be created
				std::unique_lock<std::mutex> lock{ mu };
				while (!ready)
					cv.wait(lock);
				lock.unlock();

				for (int iter = 0; iter < numIterations; ++iter)
				{
					//generate random key
					MyKey key{ -1 };
					if(generateRandomKey)
						key = MyKey{ distKey(mt) };
					else
						key = MyKey{ iter };
					size_t currentTimeout{ distTimeout(mt) };
					int behavior{ distBehavior(mt) };
					try
					{
						std::shared_ptr<MyValue> val;
						//val = cacheStatusMgr.get(key, false, 0);
						//if(val)
						//	log("CRIT", "=======================================================something wrong, value already in cache for key: ", key);
						val = cacheStatusMgr.setAndGet(key, numRetries, wait, timeoutMilliSec,
							prepareDataToCache, key.val_, currentTimeout, behavior);
					}
					catch (std::exception& e)
					{
						log("CRIT", "Exception: ", e.what(), " key: ", key);
					}
				}
			};

			int numThreads = 200;
			int numIterations = 5000;
			size_t timeoutMilliSec = 200;

			CacheType cache;
			CacheStatusMgrType cacheMgr{ cache };

			std::vector<std::thread> threadPool;
			for (int i = 0; i < numThreads; ++i)
				threadPool.push_back(std::thread{ threadFunction, std::ref(cacheMgr), numIterations, timeoutMilliSec });

			std::unique_lock<std::mutex> lock{ mu };
			ready = true;
			lock.unlock();

			cv.notify_all();

			for (int i = 0; i < numThreads; ++i)
				if (threadPool[i].joinable())
					threadPool[i].join();

			cacheMgr.clear();
		}

	}


	MM_DECLARE_FLAG(CacheStatusManager_v1);

	MM_UNIT_TEST(CacheStatusManager_v1_Test, CacheStatusManager_v1)
	{
		cacheStatusManager_v1::testCacheStatusManager(false);
		//cacheStatusManager_v1::testCacheStatusManager(true);
	}
}

