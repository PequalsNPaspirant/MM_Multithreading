/*
write example of each memory order for all atomic operations.

e.g.
Std::atomic<size_t> count{0};
bool functionThatCanBeCalledByMultipleThreads()
{
Count.fetch_add(1, std::Memory_order_relaxed);
… //Other code
… //Other code
}


*/