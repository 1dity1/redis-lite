#include <iostream>
#include <thread>
#include "store.h"
#include "server.h"
#include "../api/http_server.h"

int main(int argc, char* argv[]) {
    int port      = 6379;
    int http_port = 6380;

    if (argc >= 2) {
        try { port = std::stoi(argv[1]); }
        catch (...) { std::cerr << "Usage: ./redis-lite [port] [http_port]\n"; return 1; }
    }
    if (argc >= 3) {
        try { http_port = std::stoi(argv[2]); }
        catch (...) { std::cerr << "Usage: ./redis-lite [port] [http_port]\n"; return 1; }
    }

    KVStore store(1024);

    std::cout << "  ____          _ _          _ _ _\n";
    std::cout << " |  _ \\ ___  __| (_)___     | (_) |_ ___\n";
    std::cout << " | |_) / _ \\/ _` | / __|____| | | __/ _ \\\n";
    std::cout << " |  _ <  __/ (_| | \\__ \\____| | | ||  __/\n";
    std::cout << " |_| \\_\\___|\\__,_|_|___/    |_|_|\\__\\___|\n\n";

    // HTTP API runs in background thread
    std::thread(run_http_server, std::ref(store), http_port).detach();

    // TCP server blocks main thread
    run_server(store, port);
    return 0;
}
