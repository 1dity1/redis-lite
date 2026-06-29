#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>
#include "lru_cache.h"

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

class KVStore {
public:
    explicit KVStore(size_t capacity = 1024);
    void set(const std::string& key, const std::string& value);
    void set_ex(const std::string& key, const std::string& value, int ttl_seconds);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    int ttl(const std::string& key);
    void purge_expired();
    size_t size();

    // Atomic increment/decrement — returns new value or error string
    // delta can be negative for decrement
    std::pair<bool, long long> incr_by(const std::string& key, long long delta);

private:
    mutable std::mutex mu_;
    LRUCache cache_;
    std::unordered_map<std::string, TimePoint> ttl_map_;
    bool is_expired_locked(const std::string& key) const;
    void delete_locked(const std::string& key);
};
