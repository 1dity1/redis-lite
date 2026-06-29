#include "lru_cache.h"

LRUCache::LRUCache(size_t capacity) : capacity_(capacity) {}

void LRUCache::move_to_front(ListIt it) {
    list_.splice(list_.begin(), list_, it);
}

std::optional<std::string> LRUCache::get(const std::string& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    move_to_front(it->second);
    return it->second->second;
}

std::string LRUCache::put(const std::string& key, const std::string& value) {
    std::string evicted = "";
    auto it = map_.find(key);
    if (it != map_.end()) {
        it->second->second = value;
        move_to_front(it->second);
        return evicted;
    }
    if (map_.size() >= capacity_) {
        auto& lru_node = list_.back();
        evicted = lru_node.first;
        map_.erase(lru_node.first);
        list_.pop_back();
    }
    list_.emplace_front(key, value);
    map_[key] = list_.begin();
    return evicted;
}

void LRUCache::remove(const std::string& key) {
    auto it = map_.find(key);
    if (it == map_.end()) return;
    list_.erase(it->second);
    map_.erase(it);
}

bool LRUCache::contains(const std::string& key) const {
    return map_.count(key) > 0;
}
