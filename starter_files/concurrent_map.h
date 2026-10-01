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

#endif /* CONCURRENT_MAP_H */
