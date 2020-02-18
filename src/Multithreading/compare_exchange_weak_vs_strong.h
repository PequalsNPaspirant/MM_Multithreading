/*
The following example illustrates the difference between compare_exchange_weak() and compare_exchange_strong()

Reference:
https://www.youtube.com/watch?v=ZQFzMfHIxng&t=27s
CppCon 2017: Fedor Pikus “C++ atomics, from basic to advanced. What do they really do?”

Note:
on x86, the compare_exchange_weak() is exactly same as compare_exchange_strong(). It does not fail to get
the hardware lock and does not returns false even if this->data_ == expectedVal
(check the definition of the compare_exchange_weak() below)
*/

//The below class represents the exclusive access implemented in hardware
class HardwareLock
{
public:

};

template<typename T>
class MyAtomic
{
public:
	//Unoptimized implementation showing the behavior of compare_exchange_strong()
	bool compare_exchange_strong(T& expectedVal, T newVal)
	{
		if (data_ != expectedVal)
		{
			expectedVal = data_;
			return false;
		}

		data_ = newVal;
		return true;
	}

	//Unoptimized implementation showing how actually it works along with the hardware locks
	bool compare_exchange_strong(T& expectedVal, T newVal)
	{
		HardwareLock lock{};                 // Get exclusive hardware lock
		T temp = data_;                      // Read the current value
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}
			
		data_ = newVal;
		return true;
	}

	//Optimized implementation - reads are faster than writes
	// Double checked locking pattern!
	bool compare_exchange_strong(T& expectedVal, T newVal)
	{
		T temp = data_;                      // Read the current value
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}

		HardwareLock lock{};                 // Get exclusive hardware lock
		temp = data_;                        // Read the current value again because it might  have changed
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}
		data_ = newVal;
		return true;
	}

	//with memory barriers
	bool compare_exchange_strong(T& expectedVal, T newVal, std::memory_order on_success, std::memory_order on_failure)
	{
		T temp = data_.load(on_failure);                      // Read the current value
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}

		HardwareLock lock{};                 // Get exclusive hardware lock
		temp = data_;                        // Read the current value again because it might  have changed
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}
		data_.store(newVal, on_failure);
		return true;
	}

	//Optimized implementation
	//Does exactly same as compare_exchange_strong() but can 'spuriously fail' and return false even if this->data_ == expectedVal
	bool compare_exchange_weak(T& expectedVal, T newVal)
	{
		T temp = data_;                      // Read the current value
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}

		HardwareLock lock{};                 // Get exclusive hardware lock, but not guaranteed and can fail
		if (!lock.locked())                  // May fail to get lock/exclusive access on a particular hardware platform
			return false;
		temp = data_;                        // Read the current value again because it might  have changed
		if (temp != expectedVal)
		{
			expectedVal = data_;
			return false;
		}
		data_ = newVal;
		return true;
	}

private:
	T data_;
};