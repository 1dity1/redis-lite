#include "server.h"
#include "command.h"
#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static void handle_client(int client_fd, KVStore& store) {
    char buf[4096];
    while (true) {
        memset(buf, 0, sizeof(buf));
        int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        std::string raw(buf, n);
        std::string response = CommandParser::dispatch(raw, store);
        send(client_fd, response.c_str(), response.size(), 0);
        if (response == "+BYE\r\n") break;
    }
    close(client_fd);
}

static void ttl_purge_loop(KVStore& store) {
    using namespace std::chrono_literals;
    while (true) {
        std::this_thread::sleep_for(1s);
        store.purge_expired();
    }
}

void run_server(KVStore& store, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { std::cerr << "[error] socket() failed\n"; return; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[error] bind() failed on port " << port << "\n";
        close(server_fd); return;
    }
    if (listen(server_fd, 64) < 0) {
        std::cerr << "[error] listen() failed\n";
        close(server_fd); return;
    }

    std::cout << "[redis-lite] listening on port " << port << "\n";
    std::thread(ttl_purge_loop, std::ref(store)).detach();
    std::cout << "[redis-lite] TTL purge thread started\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) { std::cerr << "[warn] accept() failed\n"; continue; }

        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port       = ntohs(client_addr.sin_port);
        std::cout << "[redis-lite] client connected: " << client_ip << ":" << client_port << "\n";

        std::thread(handle_client, client_fd, std::ref(store)).detach();
    }
    close(server_fd);
}
