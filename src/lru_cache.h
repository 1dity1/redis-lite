#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <optional>

class LRUCache {
public:
    explicit LRUCache(size_t capacity);
    std::optional<std::string> get(const std::string& key);
    std::string put(const std::string& key, const std::string& value);
    void remove(const std::string& key);
    bool contains(const std::string& key) const;
    size_t size() const { return map_.size(); }
    size_t capacity() const { return capacity_; }
private:
    using ListIt = std::list<std::pair<std::string, std::string>>::iterator;
    size_t capacity_;
    std::list<std::pair<std::string, std::string>> list_;
    std::unordered_map<std::string, ListIt> map_;
    void move_to_front(ListIt it);
};
