/*
Reference:
https://www.youtube.com/watch?v=ZQFzMfHIxng&t=27s

Atomic list:
Atomic list:
struct node { int value; node* next; };
std::atomic<node*> head;
void push_front(int x) {
node* new_n = new node;
new_n->value = x;
node* old_h = head;
do { new_n->next = old_h; }
while (!head.compare_exchange_strong(old_h,new_n);
}

Atomic variable is a pointer to (non-atomic) memory

*/