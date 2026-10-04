// Extra correctness checks; the supplied tests and benchmark stay unchanged.
#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "concurrent_map.h"
#include "hash_map.h"
#include "locks.h"

static const char* context = "setup";
#define CHECK(x) do { if (!(x)) { \
    std::fprintf(stderr, "%s: line %d: %s\n", context, __LINE__, #x); \
    std::abort(); \
} } while (false)

// A reproducible operation stream checked against the standard container.
template<class Map>
void differential(Map& map) {
    std::map<int, int> reference;
    std::mt19937 rng(379);
    for (int i = 0; i < 6000; ++i) {
        int key = static_cast<int>(rng() % 63) - 31;
        int value = static_cast<int>(rng() % 10000);
        switch (rng() % 4) {
        case 0:
            CHECK(map.insert(key, value) == reference.insert_or_assign(key, value).second);
            break;
        case 1: {
            int out = -12345;
            auto it = reference.find(key);
            CHECK(std::as_const(map).find(key, out) == (it != reference.end()));
            CHECK(out == (it == reference.end() ? -12345 : it->second));
            break;
        }
        case 2:
            CHECK(map.erase(key) == (reference.erase(key) != 0));
            break;
        case 3:
            CHECK(std::as_const(map).size() == reference.size());
            break;
        }
    }
    CHECK(map.size() == reference.size());
    for (auto [key, value] : reference) {
        int out = 0;
        CHECK(map.find(key, out) && out == value);
        CHECK(map.erase(key));
    }
    CHECK(map.size() == 0);
}

// Each phase starts together. Exactly one caller adds/removes the shared key.
template<class Map>
void same_key(Map& map) {
    constexpr int threads = 8, rounds = 64;
    std::barrier phase(threads);
    std::atomic<int> inserted{0}, erased{0};
    std::vector<std::jthread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t] {
            for (int r = 0; r < rounds; ++r) {
                phase.arrive_and_wait();
                if (map.insert(r, t)) inserted.fetch_add(1, std::memory_order_relaxed);
                phase.arrive_and_wait();
                int out = -1;
                CHECK(map.find(r, out) && out >= 0 && out < threads);
                CHECK(map.size() == 1);
                phase.arrive_and_wait();
                if (map.erase(r)) erased.fetch_add(1, std::memory_order_relaxed);
                phase.arrive_and_wait();
                CHECK(map.size() == 0);
            }
        });
    }
    workers.clear(); //join before checking totals
    CHECK(inserted.load() == rounds && erased.load() == rounds);
}

using Clock = std::chrono::steady_clock;
struct Operation {
    int kind = 0, key = 0, value = 0; //insert, find, erase, size
    bool result = false;
    int out = -999;
    std::size_t size = 0;
    Clock::time_point begin{}, end{};
};
using Model = std::array<std::optional<int>, 3>;

bool matches(const Operation& op, Model& model) {
    auto& entry = model[op.key];
    switch (op.kind) {
    case 0: {
        bool added = !entry.has_value();
        entry = op.value;
        return op.result == added;
    }
    case 1:
        return op.result == entry.has_value() && op.out == entry.value_or(-999);
    case 2: {
        bool present = entry.has_value();
        entry.reset();
        return op.result == present;
    }
    default: {
        std::size_t size = 0;
        for (const auto& item : model) size += item.has_value();
        return op.size == size;
    }
    }
}

// Search every legal ordering as needed. Operations that finished before another
// began must precede it; overlapping operations may appear in either order.
bool linearizable(const std::array<Operation, 9>& history, unsigned done,
                  Model model, const Model& final) {
    if (done == (1u << history.size()) - 1) return model == final;
    for (unsigned i = 0; i < history.size(); ++i) {
        if (done & (1u << i)) continue;
        bool blocked = false;
        for (unsigned j = 0; j < history.size(); ++j) {
            if (!(done & (1u << j)) && history[j].end < history[i].begin) {
                blocked = true;
                break;
            }
        }
        if (blocked) continue;
        auto next = model;
        if (matches(history[i], next) &&
            linearizable(history, done | (1u << i), next, final)) return true;
    }
    return false;
}

template<class Factory>
void histories(Factory factory) {
    std::mt19937 rng(12345);
    for (int round = 0; round < 24; ++round) {
        auto map = factory();
        std::array<Operation, 9> history;
        for (unsigned i = 0; i < history.size(); ++i) {
            history[i].kind = (round + i) % 4;
            history[i].key = rng() % 3;
            history[i].value = round * 10 + i;
        }
        std::barrier start(3);
        {
            std::vector<std::jthread> workers;
            for (int t = 0; t < 3; ++t) workers.emplace_back([&, t] {
                start.arrive_and_wait();
                for (int i = t * 3; i < t * 3 + 3; ++i) {
                    auto& op = history[i];
                    // Clock timestamps avoid adding a shared atomic handoff
                    // around every operation being checked.
                    op.begin = Clock::now();
                    switch (op.kind) {
                    case 0: op.result = map->insert(op.key, op.value); break;
                    case 1: op.result = map->find(op.key, op.out); break;
                    case 2: op.result = map->erase(op.key); break;
                    case 3: op.size = map->size(); break;
                    }
                    op.end = Clock::now();
                    std::this_thread::yield();
                }
            });
        }
        Model final{};
        for (int key = 0; key < 3; ++key) {
            int out = -999;
            if (map->find(key, out)) final[key] = out;
        }
        if (!linearizable(history, 0, {}, final)) {
            std::fprintf(stderr, "history round %d failed\n", round);
            for (const auto& op : history) {
                std::fprintf(stderr, "kind=%d key=%d value=%d result=%d out=%d size=%zu begin=%lld end=%lld\n",
                    op.kind, op.key, op.value, op.result, op.out, op.size,
                    static_cast<long long>(op.begin.time_since_epoch().count()),
                    static_cast<long long>(op.end.time_since_epoch().count()));
            }
            CHECK(false);
        }
    }
}

void check_oracle() {
    std::array<Operation, 9> h{};
    for (auto& op : h) op.kind = 3; //all size() calls see an empty map
    CHECK(linearizable(h, 0, {}, {}));
    h[0].size = 1;
    CHECK(!linearizable(h, 0, {}, {}));
    h[0].kind = 0;
    h[0].result = true;
    h[0].value = 42;
    h[0].begin = Clock::time_point{} + std::chrono::seconds(1);
    h[0].end = h[0].begin + std::chrono::seconds(1);
    for (unsigned i = 1; i < h.size(); ++i) {
        h[i].begin = h[0].end + std::chrono::seconds(1);
        h[i].end = h[i].begin + std::chrono::seconds(1);
    }
    Model final{};
    final[0] = 42;
    CHECK(!linearizable(h, 0, {}, final)); //later size() must not return zero
    for (unsigned i = 1; i < h.size(); ++i) h[i].size = 1;
    CHECK(linearizable(h, 0, {}, final));
}

template<class Factory>
void exercise(const char* name, Factory factory) {
    context = name;
    std::printf("%s\n", name);
    { auto map = factory(); differential(*map); same_key(*map); }
    histories(factory);
}

template<class Lock, bool Padded>
void combination(const char* lock_name) {
    char name[100];
    std::snprintf(name, sizeof(name), "sharded/%s/padded=%d", lock_name, Padded);
    exercise(name, [] { return std::make_unique<ShardedMap<int, int, Lock, Padded>>(3); });
    std::snprintf(name, sizeof(name), "hashed/%s/padded=%d", lock_name, Padded);
    exercise(name, [] { return std::make_unique<StripedHashMap<int, int, Lock, Padded>>(7, 3); });
}

template<class Lock>
void both_padding_modes(const char* name) {
    static_assert(!std::is_copy_constructible_v<Lock>);
    static_assert(!std::is_move_constructible_v<Lock>);
    combination<Lock, true>(name);
    combination<Lock, false>(name);
}

struct Value {
    static inline int live = 0;
    static inline bool fail = false;
    int value;
    explicit Value(int v) : value(v) { ++live; }
    Value(const Value& other) : value(other.value) {
        if (fail) throw std::runtime_error("copy");
        ++live;
    }
    Value& operator=(const Value& other) {
        if (fail) throw std::runtime_error("assignment");
        value = other.value;
        return *this;
    }
    ~Value() { --live; }
};

template<class Factory>
void exceptions(Factory factory) {
    {
        auto map = factory();
        Value value(42), out(0);
        Value::fail = true;
        try { map->insert(1, value); CHECK(false); }
        catch (const std::runtime_error&) { }
        Value::fail = false;
        CHECK(map->size() == 0 && Value::live == 2);
        CHECK(map->insert(1, value));
        Value::fail = true;
        try { map->insert(1, value); CHECK(false); }
        catch (const std::runtime_error&) { }
        try { map->find(1, out); CHECK(false); }
        catch (const std::runtime_error&) { }
        Value::fail = false;
        CHECK(map->find(1, out) && out.value == 42);
        for (int i = 2; i < 2000; ++i) CHECK(map->insert(i, value));
        CHECK(map->erase(1));
    }
    CHECK(Value::live == 0);
}

void hash_edges() {
    context = "hash edges";
    for (unsigned buckets : {1u, 7u, 8u}) {
        for (unsigned stripes : {1u, 3u, 16u}) {
            StripedHashMap<int, int, TTASLock, false> map(buckets, stripes);
            CHECK(map.bucket_count() == buckets && map.stripe_count() == stripes);
            for (int i = 0; i < 4; ++i) CHECK(map.insert(i * buckets, i));
            CHECK(map.erase(2 * buckets)); //middle of chain
            CHECK(map.erase(3 * buckets)); //head
            CHECK(map.erase(0));           //tail
            int out = -1;
            CHECK(map.find(buckets, out) && out == 1);
            CHECK(map.erase(buckets));
            CHECK(map.size() == 0);
            differential(map);
        }
    }
    for (auto counts : {std::pair{0u, 1u}, std::pair{1u, 0u}}) {
        try { StripedHashMap<int, int> map(counts.first, counts.second); CHECK(false); }
        catch (const std::invalid_argument&) { }
    }
}

template<class Lock>
void reader_overlap() {
    Lock lock;
    lock.lock_shared();
    std::atomic<bool> entered{false};
    std::jthread reader([&] {
        std::shared_lock guard(lock);
        entered.store(true);
    });
    auto deadline = Clock::now() + std::chrono::seconds(5);
    while (!entered.load() && Clock::now() < deadline) std::this_thread::yield();
    bool overlapped = entered.load();
    lock.unlock_shared();
    reader.join();
    CHECK(overlapped);
}

int main() {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    check_oracle();
    exercise("coarse", [] { return std::make_unique<CoarseMap<int, int>>(); });
    both_padding_modes<std::mutex>("mutex");
    both_padding_modes<std::shared_mutex>("shared_mutex");
    both_padding_modes<TASLock>("tas");
    both_padding_modes<TTASLock>("ttas");
    both_padding_modes<TicketLock>("ticket");
    both_padding_modes<ParkingLock>("park");
    both_padding_modes<RWLock>("rw");
    both_padding_modes<RWLockWP>("rwp");
    hash_edges();
    context = "exceptions and cleanup";
    exceptions([] { return std::make_unique<CoarseMap<int, Value>>(); });
    exceptions([] { return std::make_unique<ShardedMap<int, Value, TTASLock>>(3); });
    exceptions([] { return std::make_unique<StripedHashMap<int, Value, TTASLock>>(1, 3); });
    context = "shared reader overlap";
    reader_overlap<RWLock>();
    reader_overlap<RWLockWP>();
    std::puts("all extended checks passed (33 map/lock/padding combinations, 792 histories)");
}
