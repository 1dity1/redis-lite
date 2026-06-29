#include "store.h"
#include <vector>
#include <stdexcept>

KVStore::KVStore(size_t capacity) : cache_(capacity) {}

bool KVStore::is_expired_locked(const std::string& key) const {
    auto it = ttl_map_.find(key);
    if (it == ttl_map_.end()) return false;
    return Clock::now() >= it->second;
}

void KVStore::delete_locked(const std::string& key) {
    cache_.remove(key);
    ttl_map_.erase(key);
}

void KVStore::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mu_);
    cache_.put(key, value);
    ttl_map_.erase(key);
}

void KVStore::set_ex(const std::string& key, const std::string& value, int ttl_seconds) {
    std::lock_guard<std::mutex> lock(mu_);
    cache_.put(key, value);
    if (ttl_seconds > 0)
        ttl_map_[key] = Clock::now() + std::chrono::seconds(ttl_seconds);
    else
        ttl_map_.erase(key);
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mu_);
    if (is_expired_locked(key)) { delete_locked(key); return std::nullopt; }
    return cache_.get(key);
}

bool KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!cache_.contains(key)) return false;
    delete_locked(key);
    return true;
}

bool KVStore::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mu_);
    if (is_expired_locked(key)) { delete_locked(key); return false; }
    return cache_.contains(key);
}

int KVStore::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!cache_.contains(key)) return -2;
    if (is_expired_locked(key)) { delete_locked(key); return -2; }
    auto it = ttl_map_.find(key);
    if (it == ttl_map_.end()) return -1;
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        it->second - Clock::now()
    );
    return static_cast<int>(remaining.count());
}

void KVStore::purge_expired() {
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<std::string> to_delete;
    for (auto& [key, expiry] : ttl_map_)
        if (Clock::now() >= expiry)
            to_delete.push_back(key);
    for (auto& key : to_delete)
        delete_locked(key);
}

size_t KVStore::size() {
    std::lock_guard<std::mutex> lock(mu_);
    return cache_.size();
}

// Atomic increment by delta (can be negative for decrement)
// Returns {true, new_value} on success
// Returns {false, 0} if value is not an integer
std::pair<bool, long long> KVStore::incr_by(const std::string& key, long long delta) {
    std::lock_guard<std::mutex> lock(mu_);

    long long current = 0;

    if (cache_.contains(key) && !is_expired_locked(key)) {
        auto val = cache_.get(key);
        try {
            current = std::stoll(*val);
        } catch (...) {
            return {false, 0};  // not an integer
        }
    }

    current += delta;
    cache_.put(key, std::to_string(current));
    return {true, current};
}
