

/*
https://www.geeksforgeeks.org/lock-variable-synchronization-mechanism/

char buffer[SIZE]; 
int count = 0, 
    start = 0, 
    end = 0; 
struct lock l; 
  
// initialize lock variable 
lock_init(&l); 
  
void put(char c) 
{ 
  
    // entry section 
    lock_acquire(&l); 
  
    // critical section begins 
    while (count == SIZE) { 
  
        lock_release(&l); 
        lock_acquire(&l); 
    } 
  
    count++; 
    buffer[start] = c; 
    start++; 
  
    if (start == SIZE) { 
  
        start = 0; 
    } 
  
    // critical section ends 
    // exit section 
    lock_release(&l); 
} 
  
char get() 
{ 
  
    char c; 
  
    // entry section 
    lock_acquire(&l); 
  
    // critical section begins 
    while (count == 0) { 
  
        lock_release(&l); 
        lock_acquire(&l); 
    } 
  
    count--; 
    c = buffer[end]; 
    end++; 
  
    if (end == SIZE) { 
  
        end = 0; 
    } 
  
    // critical section ends 
    // exit section 
    lock_release(&l); 
  
    return c; 
} 



*/