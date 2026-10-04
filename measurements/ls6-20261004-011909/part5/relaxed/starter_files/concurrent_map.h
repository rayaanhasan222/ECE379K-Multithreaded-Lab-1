// concurrent_map.h -- Lab 1: Part 1 and Part 4.  YOU WRITE THIS FILE.
//
// Nothing compiles until CoarseMap exists, so it is the first thing
// to write.  What the harness and tests expect from this header:
//
//   template <typename K, typename V>
//   class CoarseMap;                               // Part 1
//
//   template <typename K, typename V,
//             class Lock = std::mutex, bool Padded = true>
//       requires BasicLock<Lock>
//   class ShardedMap;                              // Part 4
//       explicit ShardedMap(std::size_t nshards);
//       std::size_t shard_count() const;
//
// Both must satisfy ConcurrentMap<M, K, V> from interface.h:
//
//   bool insert(const K& key, const V& value);   // true if key was new
//   bool find  (const K& key, V& out) const;     // copy; false if absent
//   bool erase (const K& key);                   // true if was present
//   std::size_t size() const;
//
// Values come out as COPIES, never references or iterators into the
// container.  The report asks why.
//
// Flip HAVE_SHARDED in parts.h when ShardedMap compiles.

#ifndef CONCURRENT_MAP_H
#define CONCURRENT_MAP_H

#include <cstddef>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "interface.h"

template <typename K, typename V>
class CoarseMap {
public:
    bool insert(const K& key, const V& value) {
        std::lock_guard guard(mutex_);
        return map_.insert_or_assign(key, value).second;
    }

    bool find(const K& key, V& out) const {
        std::lock_guard guard(mutex_);
        const auto it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    bool erase(const K& key) {
        std::lock_guard guard(mutex_);
        return map_.erase(key) != 0;
    }

    std::size_t size() const {
        std::lock_guard guard(mutex_);
        return map_.size();
    }

private:
    // Every access to map_, including reads, holds this mutex.
    // Const operations still acquire the lock and copy values before releasing it.
    mutable std::mutex mutex_;
    std::map<K, V> map_;
};

template <typename K, typename V, class Lock = std::mutex, bool Padded = true>
    requires BasicLock<Lock>
class ShardedMap {
private:
    using Map = std::map<K, V>;
    static constexpr std::size_t NATURAL_ALIGNMENT =
        alignof(Lock) > alignof(Map) ? alignof(Lock) : alignof(Map);
    static constexpr std::size_t SHARD_ALIGNMENT =
        Padded && CACHE_LINE > NATURAL_ALIGNMENT ? CACHE_LINE : NATURAL_ALIGNMENT;

    // Padding separates whole shards, including their locks and map metadata.
    struct alignas(SHARD_ALIGNMENT) Shard {
        mutable Lock lock;
        Map map;
    };
    static_assert(!Padded || sizeof(Shard) % CACHE_LINE == 0);

    static std::size_t checked_count(std::size_t count) {
        if (count == 0) {
            throw std::invalid_argument("shard count must be positive");
        }
        return count;
    }

public:
    explicit ShardedMap(std::size_t nshards) : shards_(checked_count(nshards)) {}

    // Shards own locks; their storage and lock identities stay fixed.
    ShardedMap(const ShardedMap&) = delete;
    ShardedMap& operator=(const ShardedMap&) = delete;
    ShardedMap(ShardedMap&&) = delete;
    ShardedMap& operator=(ShardedMap&&) = delete;

    bool insert(const K& key, const V& value) {
        auto& shard = shards_[shard_for(key)];
        std::lock_guard guard(shard.lock);
        return shard.map.insert_or_assign(key, value).second;
    }

    bool find(const K& key, V& out) const {
        const auto& shard = shards_[shard_for(key)];
        ReadGuard<Lock> guard(shard.lock);
        const auto it = shard.map.find(key);
        if (it == shard.map.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    bool erase(const K& key) {
        auto& shard = shards_[shard_for(key)];
        std::lock_guard guard(shard.lock);
        return shard.map.erase(key) != 0;
    }

    std::size_t size() const {
        std::vector<std::unique_lock<Lock>> guards;
        guards.reserve(shards_.size());
        // A common acquisition order prevents cycles between size() callers.
        // Single-shard operations never wait while holding another shard lock.
        for (const auto& shard : shards_) {
            guards.emplace_back(shard.lock);
        }

        // With every lock held, the sum describes one consistent map state.
        std::size_t total = 0;
        for (const auto& shard : shards_) {
            total += shard.map.size();
        }
        return total;
    }

    std::size_t shard_count() const noexcept { return shards_.size(); }

private:
    std::size_t shard_for(const K& key) const {
        return std::hash<K>{}(key) % shards_.size();
    }

    std::vector<Shard> shards_;
};

#endif /* CONCURRENT_MAP_H */
