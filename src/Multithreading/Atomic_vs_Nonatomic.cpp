/*
Write a test to increment a normal int and atomic int by multiple threads and see the end value
Lets say we have 100 threads and each thread increments the value 50 times, then the final value of int should be:
100 * 50 = 5000 if the initial value was zero.
Do this test for different data sizes like char, short, int, long, double, unsigned double etc and also for simple struct with different combinations
of data structures to see what is the effect of padding on this.
*/