cmake_minimum_required(VERSION 3.15)
project(redis-lite CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# Main server binary
add_executable(redis-lite
    src/main.cpp
    src/server.cpp
    src/store.cpp
    src/command.cpp
    src/lru_cache.cpp
    api/http_server.cpp
)
find_package(Threads REQUIRED)
target_link_libraries(redis-lite Threads::Threads)
target_compile_options(redis-lite PRIVATE -Wall -Wextra -Wpedantic)

# Tests
enable_testing()
find_package(GTest REQUIRED)

add_executable(run_tests
    tests/test_lru.cpp
    tests/test_store.cpp
    tests/test_command.cpp
    src/lru_cache.cpp
    src/store.cpp
    src/command.cpp
)
target_include_directories(run_tests PRIVATE src)
target_link_libraries(run_tests GTest::GTest GTest::Main Threads::Threads)
add_test(NAME all_tests COMMAND run_tests)
