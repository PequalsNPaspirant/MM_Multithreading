/*
Reference: https://webkit.org/blog/6161/locking-in-webkit/

class Spinlock {
public:
	Spinlock()
	{
		m_isLocked.store(false);
	}

	void lock()
	{
		while (!m_isLocked.compareExchangeWeak(false, true)) {}
	}

	void unlock()
	{
		m_isLocked.store(false);
	}
private:
	Atomic<bool> m_isLocked;
};

*/