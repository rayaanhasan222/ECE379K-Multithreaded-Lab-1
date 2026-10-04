// Reuse the provided size test without changing it or running the other cases.
#define main supplied_test_main
#include "test_map.cpp"
#undef main

class ObservedMap {
public:
    bool insert(long key, long value) {
        bool result = map_.insert(key, value);
        inserts.fetch_add(1, std::memory_order_relaxed);
        return result;
    }
    bool erase(long key) {
        bool result = map_.erase(key);
        erases.fetch_add(1, std::memory_order_relaxed);
        return result;
    }
    std::size_t size() const {
        auto result = map_.size();
        sizes.fetch_add(1, std::memory_order_relaxed);
        return result;
    }

    std::atomic<unsigned long> inserts{0}, erases{0};
    mutable std::atomic<unsigned long> sizes{0};

private:
    StripedHashMap<long, long, std::mutex> map_{1 << 16, 1};
};

int main() {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    ObservedMap map;
    // These counters are diagnostic only; observing progress can change timing.
    // They count completed calls, not attempted lock acquisitions.
    auto start = std::chrono::steady_clock::now();
    std::jthread observer([&](std::stop_token stop) {
        while (!stop.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (stop.stop_requested()) break;
            auto elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start).count();
            std::printf("%.1fs: completed inserts=%lu erases=%lu size_calls=%lu\n",
                elapsed, map.inserts.load(std::memory_order_relaxed),
                map.erases.load(std::memory_order_relaxed),
                map.sizes.load(std::memory_order_relaxed));
        }
    });
    test_size_exact(map, "isolated hash map, 65536 buckets, one mutex", true);
    observer.request_stop();
    observer.join();
    std::printf("final: inserts=%lu erases=%lu size_calls=%lu\n",
        map.inserts.load(), map.erases.load(), map.sizes.load());
    if (failures) return 1;
    std::puts("isolated size test passed");
}
