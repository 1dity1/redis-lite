#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "store.h"

// Basic set/get
TEST(KVStore, BasicSetGet) {
    KVStore store(10);
    store.set("name", "aditya");
    EXPECT_EQ(store.get("name").value(), "aditya");
}

// Missing key
TEST(KVStore, MissingKey) {
    KVStore store(10);
    EXPECT_FALSE(store.get("ghost").has_value());
}

// Overwrite key
TEST(KVStore, Overwrite) {
    KVStore store(10);
    store.set("k", "v1");
    store.set("k", "v2");
    EXPECT_EQ(store.get("k").value(), "v2");
}

// DEL
TEST(KVStore, Del) {
    KVStore store(10);
    store.set("k", "v");
    EXPECT_TRUE(store.del("k"));
    EXPECT_FALSE(store.get("k").has_value());
    EXPECT_FALSE(store.del("k"));  // already deleted
}

// EXISTS
TEST(KVStore, Exists) {
    KVStore store(10);
    store.set("k", "v");
    EXPECT_TRUE(store.exists("k"));
    store.del("k");
    EXPECT_FALSE(store.exists("k"));
}

// TTL expiry — key should vanish after TTL
TEST(KVStore, TTLExpiry) {
    KVStore store(10);
    store.set_ex("session", "abc", 1);  // 1 second TTL
    EXPECT_TRUE(store.get("session").has_value());
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_FALSE(store.get("session").has_value());  // expired
}

// TTL on key with no expiry returns -1
TEST(KVStore, TTLNoExpiry) {
    KVStore store(10);
    store.set("k", "v");
    EXPECT_EQ(store.ttl("k"), -1);
}

// TTL on missing key returns -2
TEST(KVStore, TTLMissingKey) {
    KVStore store(10);
    EXPECT_EQ(store.ttl("ghost"), -2);
}

// SET clears existing TTL
TEST(KVStore, SetClearsTTL) {
    KVStore store(10);
    store.set_ex("k", "v", 5);
    store.set("k", "v2");           // overwrite without TTL
    EXPECT_EQ(store.ttl("k"), -1);  // TTL should be gone
}

// purge_expired removes expired keys
TEST(KVStore, PurgeExpired) {
    KVStore store(10);
    store.set_ex("temp", "val", 1);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    store.purge_expired();
    EXPECT_EQ(store.size(), 0);
}

// Concurrent writes — no crash
TEST(KVStore, ConcurrentWrites) {
    KVStore store(1000);
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&store, i]() {
            for (int j = 0; j < 100; j++) {
                store.set("key" + std::to_string(i), std::to_string(j));
                store.get("key" + std::to_string(i));
            }
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_LE(store.size(), 10);
}
