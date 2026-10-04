// locks.h -- Lab 1: Part 5 and Part 6.  YOU WRITE THIS FILE.
//
// What the harness and tests expect from this header:
//
//   Part 5 (BasicLock):     TASLock  TTASLock  TicketLock  ParkingLock
//   Part 6 (SharedLock):    RWLock   RWLockWP
//
// Every lock is a class with a default constructor and
//
//   void lock();
//   void unlock();
//
// and the two reader-writer locks additionally
//
//   void lock_shared();
//   void unlock_shared();
//
// so that std::lock_guard, std::unique_lock, std::shared_lock, and
// the lab's ReadGuard all work on them.  A lock must not be copyable
// or movable (std::atomic already sees to that).
//
// Everything in here is built from std::atomic.  No std::mutex, no
// std::shared_mutex, no OS primitives except the ones behind
// std::atomic::wait / notify_one, which ParkingLock uses.
//
// Flip HAVE_LOCKS (Part 5) and HAVE_RW (Part 6) in parts.h as each
// set compiles.

#ifndef LOCKS_H
#define LOCKS_H

#include <atomic>
#include <cstdint>
#include <thread>

#include "interface.h"

namespace lab1_detail {
// A processor spin hint, not a scheduler yield or a memory fence.
inline void spin_hint() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    __asm__ __volatile__("yield");
#endif
}
} // namespace lab1_detail, used generative AI here for different architectures

class TASLock {
public:
    void lock() noexcept {
        while (held_.exchange(true, std::memory_order_relaxed)) {
            lab1_detail::spin_hint();
        }
    }

    void unlock() noexcept {
        held_.store(false, std::memory_order_relaxed);
    }

private:
    std::atomic<bool> held_{false}; //false means it is free, true means it is held
};

class TTASLock {
public:
    void lock() noexcept {
        unsigned backoff = 1;
        for (;;) { //unconditional loop
            //polling does not grant ownership; only the exchange does. many other threads can see false simulatenosuly 
            while (held_.load(std::memory_order_relaxed)) {
                lab1_detail::spin_hint();
            }
            if (!held_.exchange(true, std::memory_order_acquire)) {
                return;
            }
            for (unsigned i = 0; i < backoff; ++i) {
                lab1_detail::spin_hint();
            }
            if (backoff < MAX_BACKOFF) {
                backoff *= 2; //reduces how aggressively losing threads retry for contention
            }
        }
    }

    void unlock() noexcept {
        held_.store(false, std::memory_order_release);
    }

private:
    static constexpr unsigned MAX_BACKOFF = 64;
    std::atomic<bool> held_{false};
};

class TicketLock {
public:
    void lock() noexcept {
        const auto ticket = next_.fetch_add(1, std::memory_order_relaxed);
        unsigned spins = 0;
        //acquiring the serving counter makes the previous owner's work visible
        while (serving_.load(std::memory_order_acquire) != ticket) {
            lab1_detail::spin_hint();
            if (++spins == SPINS_BEFORE_YIELD) {
                //FIFO waiters cannot bypass a descheduled next-in-line thread
                std::this_thread::yield();
                spins = 0;
            }
        }
    }

    void unlock() noexcept {
        //only the current owner advances serving_. Unsigned wrap is defined
        const auto current = serving_.load(std::memory_order_relaxed);
        serving_.store(current + 1, std::memory_order_release);
    }

private:
    static constexpr unsigned SPINS_BEFORE_YIELD = 64;
    std::atomic<std::uint32_t> next_{0};
    std::atomic<std::uint32_t> serving_{0};
};

class ParkingLock {
public:
    void lock() noexcept {
        int expected = 0;
        if (state_.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
            return;
        }

        for (unsigned i = 0; i < SPINS_BEFORE_WAIT; ++i) {
            if (state_.load(std::memory_order_relaxed) == 0) {
                expected = 0;
                if (state_.compare_exchange_weak(expected, 1, std::memory_order_acquire, std::memory_order_relaxed)) {
                    return;
                }
            }
            lab1_detail::spin_hint();
        }

        //mark possible sleepers before waiting. A slow-path owner keeps state 2 so its unlock wakes another waiter, even if others are still asleep.
        while (state_.exchange(2, std::memory_order_acquire) != 0) {
            // wait checks the value before sleeping; an intervening unlock
            // cannot strand us waiting for an already-completed notification.
            state_.wait(2, std::memory_order_relaxed);
        }
    }

    void unlock() noexcept {
        if (state_.exchange(0, std::memory_order_release) == 2) {
            state_.notify_one();
        }
    }

private:
    static constexpr unsigned SPINS_BEFORE_WAIT = 64;
    //0: free; 1: held; 2: held with possible sleeping waiters
    std::atomic<int> state_{0};
};

class RWLock {
public:
    void lock() noexcept {
        for (;;) {
            int expected = 0;
            //a writer can only enter when there are no readers or another writer
            if (state_.compare_exchange_weak(expected, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }
            lab1_detail::spin_hint();
        }
    }

    void unlock() noexcept {
        state_.store(0, std::memory_order_release);
    }

    void lock_shared() noexcept {
        for (;;) {
            int readers = state_.load(std::memory_order_relaxed);
            //readers can join other readers, but cannot enter while a writer holds the lock
            if (readers >= 0 && state_.compare_exchange_weak(readers, readers + 1, std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }
            lab1_detail::spin_hint();
        }
    }

    void unlock_shared() noexcept {
        //remove only this reader, the last reader changes the count back to 0
        state_.fetch_sub(1, std::memory_order_release);
    }

private:
    std::atomic<int> state_{0}; //-1 means a writer holds it, otherwise it is the number of readers
};

class RWLockWP {
public:
    void lock() noexcept {
        //announce that a writer is waiting so new readers stop entering
        waiting_writers_.fetch_add(1, std::memory_order_seq_cst);
        for (;;) {
            int expected = 0;
            if (state_.compare_exchange_weak(expected, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
                //we now own the lock, so we are no longer a waiting writer
                waiting_writers_.fetch_sub(1, std::memory_order_seq_cst);
                return;
            }
            lab1_detail::spin_hint();
        }
    }

    void unlock() noexcept {
        state_.store(0, std::memory_order_release);
    }

    void lock_shared() noexcept {
        for (;;) {
            while (waiting_writers_.load(std::memory_order_seq_cst) != 0) {
                lab1_detail::spin_hint();
            }

            int readers = state_.load(std::memory_order_relaxed);
            if (readers >= 0 && state_.compare_exchange_weak(readers, readers + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
                //a writer may have arrived between our first check and reserving a reader slot
                if (waiting_writers_.load(std::memory_order_seq_cst) == 0) {
                    return;
                }
                //give the slot back before touching protected data, then wait for the writer
                state_.fetch_sub(1, std::memory_order_release);
            }
            lab1_detail::spin_hint();
        }
    }

    void unlock_shared() noexcept {
        state_.fetch_sub(1, std::memory_order_release);
    }

private:
    std::atomic<int> state_{0}; //-1 means a writer holds it, otherwise it is the number of readers
    //seq_cst orders writer announcements and reader admission checks across both atomics
    std::atomic<unsigned> waiting_writers_{0};
};

#endif /* LOCKS_H */
