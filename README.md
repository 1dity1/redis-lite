# redis-lite

A Redis-compatible in-memory key-value store, built from scratch in C++17 over a weekend as a systems programming exercise.

Not a production Redis replacement — but a ground-up implementation of the core ideas: custom TCP protocol, LRU eviction, TTL expiry, and a REST API layer on top.

## What it does

- Key-value store over a raw TCP connection (port 6379)
- TTL support with both lazy deletion and background purge
- LRU eviction when the store hits capacity (doubly linked list + hashmap, O(1))
- Atomic counters via INCR/DECR/INCRBY/DECRBY
- HTTP API on port 6380 for browser-based access
- React dashboard with live key count, command panel, and a chart
- 40 unit tests across LRU cache, KV store, and command parser
- Benchmarks at roughly 11k req/sec under 10 concurrent clients

## Building

```bash
mkdir build && cd build
cmake .. && make -j4
./redis-lite
```

## Running tests

```bash
cd build
./run_tests
```

## Benchmark

```bash
python3 benchmark.py
```

## Supported commands
SET key value

GET key

DEL key

EXISTS key

SETEX key seconds value

TTL key

INCR key

DECR key

INCRBY key delta

DECRBY key delta

PING

DBSIZE

QUIT

## Stack

C++17, CMake, POSIX sockets, Google Test, React, TypeScript, Recharts
