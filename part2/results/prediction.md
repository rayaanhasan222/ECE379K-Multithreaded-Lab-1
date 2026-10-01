# Part 2: prediction before measurement

Recorded before running the coarse-map sweep.

Lonestar6 adaptation, also before measurement: use a socket boundary of 64
and a core count of 128. The first sampled point using the second socket is
96 threads, and the oversubscribed points are 192 and 256. The qualitative
predictions below apply with those boundaries substituted for Frontera's.

I predict that throughput will peak at one or two threads, with one thread
the most likely peak. All operations, including reads, acquire the same
mutex, so extra cores cannot execute map operations concurrently. A second
thread might hide a stall, but increasing the thread count should generally
reduce throughput as mutex contention, cache-line handoffs, and scheduling
overhead grow.

At 28 threads the first socket is full. The 40-thread measurement is the
first point using the second socket; I expect a possible further drop there
because ownership of the lock and protected data can cross sockets. I do
not expect a throughput increase proportional to the available cores.

At 56 threads each thread has a physical core. At 84 and 112 threads the
machine is oversubscribed. I predict throughput will remain low or decrease
further because threads are time-sliced and a lock holder can be descheduled.
Since std::mutex can put waiters to sleep, I do not predict an exact cliff
at 56 threads. These are hypotheses; the measured curve must establish
whether the socket boundary and oversubscription actually change its shape.
