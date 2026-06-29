#include <gtest/gtest.h>
#include "lru_cache.h"

// Basic get/put
TEST(LRUCache, BasicSetGet) {
    LRUCache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    EXPECT_EQ(cache.get("a").value(), "1");
    EXPECT_EQ(cache.get("b").value(), "2");
}

// Missing key returns nullopt
TEST(LRUCache, MissingKey) {
    LRUCache cache(3);
    EXPECT_FALSE(cache.get("x").has_value());
}

// Eviction — LRU key should be evicted
TEST(LRUCache, EvictsLRU) {
    LRUCache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");
    // access "a" and "b" → "c" becomes LRU
    cache.get("a");
    cache.get("b");
    // insert "d" → "c" should be evicted
    cache.put("d", "4");
    EXPECT_FALSE(cache.get("c").has_value());
    EXPECT_TRUE(cache.get("a").has_value());
    EXPECT_TRUE(cache.get("b").has_value());
    EXPECT_TRUE(cache.get("d").has_value());
}

// Update existing key — should not evict anything
TEST(LRUCache, UpdateDoesNotEvict) {
    LRUCache cache(2);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("a", "updated");  // update, not insert
    EXPECT_EQ(cache.size(), 2);
    EXPECT_EQ(cache.get("a").value(), "updated");
    EXPECT_TRUE(cache.get("b").has_value());
}

// Remove key
TEST(LRUCache, Remove) {
    LRUCache cache(3);
    cache.put("a", "1");
    cache.remove("a");
    EXPECT_FALSE(cache.get("a").has_value());
    EXPECT_EQ(cache.size(), 0);
}

// Contains
TEST(LRUCache, Contains) {
    LRUCache cache(3);
    cache.put("x", "val");
    EXPECT_TRUE(cache.contains("x"));
    EXPECT_FALSE(cache.contains("y"));
}

// Capacity respected
TEST(LRUCache, CapacityRespected) {
    LRUCache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");
    cache.put("d", "4");
    EXPECT_EQ(cache.size(), 3);
}
