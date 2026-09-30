// hash_map.h -- Lab 1: Part 7.  YOU WRITE THIS FILE.
//
// What the harness and tests expect from this header:
//
//   template <typename K, typename V,
//             class Lock = std::mutex, bool Padded = true>
//       requires BasicLock<Lock>
//   class StripedHashMap;
//       explicit StripedHashMap(std::size_t nbuckets,
//                               std::size_t nstripes);
//       std::size_t bucket_count() const;
//       std::size_t stripe_count() const;
//
// satisfying ConcurrentMap<M, K, V> from interface.h.  The bucket
// count is fixed at construction: there is no resizing.  The tests
// construct one with 7 buckets and 3 stripes and fill it with
// thousands of keys, so long chains must work, just slowly.
//
// Flip HAVE_HASHED in parts.h when it compiles.

#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <vector>

#include "interface.h"

template <typename K, typename V, class Lock = std::mutex, bool Padded = true>
    requires BasicLock<Lock>
class StripedHashMap {
private:
    struct Node {
        K key;
        V value;
        Node* next;
    };

    //padded locks start on separate cache lines, unpadded locks sit next to each other
    struct alignas(Padded ? CACHE_LINE : alignof(Lock)) Stripe {
        Lock lock;
    };
    static_assert(!Padded || sizeof(Stripe) % CACHE_LINE == 0);

    static std::size_t checked_count(std::size_t count) {
        if (count == 0) {
            throw std::invalid_argument("bucket and stripe counts must be positive");
        }
        return count;
    }

public:
    explicit StripedHashMap(std::size_t nbuckets, std::size_t nstripes)
        : buckets_(checked_count(nbuckets), nullptr),
          stripes_(checked_count(nstripes)),
          buckets_per_stripe_(nbuckets / nstripes + (nbuckets % nstripes != 0)) {
        //round up so leftover buckets are covered, for 7 buckets and 3 stripes we get 3, 3, 1
    }

    ~StripedHashMap() {
        //all threads must finish using the map before it is destroyed
        for (Node* head : buckets_) {
            while (head != nullptr) {
                Node* next = head->next;
                delete head;
                head = next;
            }
        }
    }

    //copying or moving would need to transfer both node ownership and locks
    StripedHashMap(const StripedHashMap&) = delete;
    StripedHashMap& operator=(const StripedHashMap&) = delete;
    StripedHashMap(StripedHashMap&&) = delete;
    StripedHashMap& operator=(StripedHashMap&&) = delete;

    bool insert(const K& key, const V& value) {
        const auto bucket = bucket_for(key);
        std::lock_guard guard(stripes_[stripe_for(bucket)].lock);

        for (Node* node = buckets_[bucket]; node != nullptr; node = node->next) {
            if (node->key == key) {
                node->value = value;
                return false; //updated an existing key, no new node was added
            }
        }

        //add at the front of the chain, the old head becomes the next node
        buckets_[bucket] = new Node{key, value, buckets_[bucket]};
        return true;
    }

    bool find(const K& key, V& out) const {
        const auto bucket = bucket_for(key);
        ReadGuard<Lock> guard(stripes_[stripe_for(bucket)].lock);

        for (const Node* node = buckets_[bucket]; node != nullptr; node = node->next) {
            if (node->key == key) {
                out = node->value; //copy while holding the lock, never return a reference to a node
                return true;
            }
        }
        return false;
    }

    bool erase(const K& key) {
        const auto bucket = bucket_for(key);
        std::lock_guard guard(stripes_[stripe_for(bucket)].lock);

        //link points to the pointer that leads to the current node, including the bucket head
        Node** link = &buckets_[bucket];
        while (*link != nullptr) {
            Node* node = *link;
            if (node->key == key) {
                *link = node->next; //bypass this node before freeing it
                delete node;
                return true;
            }
            link = &node->next;
        }
        return false;
    }

    std::size_t size() const {
        std::vector<std::unique_lock<Lock>> guards;
        guards.reserve(stripes_.size());
        //keep every lock until counting is done, always acquire them in index order
        for (auto& stripe : stripes_) {
            guards.emplace_back(stripe.lock);
        }

        std::size_t total = 0;
        for (const Node* head : buckets_) {
            for (const Node* node = head; node != nullptr; node = node->next) {
                ++total;
            }
        }
        return total;
    }

    std::size_t bucket_count() const noexcept { return buckets_.size(); }
    std::size_t stripe_count() const noexcept { return stripes_.size(); }

private:
    std::size_t bucket_for(const K& key) const {
        return std::hash<K>{}(key) % buckets_.size();
    }

    std::size_t stripe_for(std::size_t bucket) const noexcept {
        return bucket / buckets_per_stripe_;
    }

    //bucket heads can still share cache lines across stripe boundaries, even with padded locks
    std::vector<Node*> buckets_;
    mutable std::vector<Stripe> stripes_; //const find() and size() still need to acquire locks
    const std::size_t buckets_per_stripe_;
};

#endif /* HASH_MAP_H */
