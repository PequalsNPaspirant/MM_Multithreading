/*
Reference:
https://www.youtube.com/watch?v=ZQFzMfHIxng&t=27s

Lock free queue:

int q[N];
std::atomic<size_t> front;
void push(int x)
{
size_t my_slot = front.fetch_add(1);
q[my_slot] = x;
}

Atomic variable is an index on non-atomic memory
*/