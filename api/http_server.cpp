#include "http_server.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// ─── Tiny HTTP helpers ───────────────────────────────────────────────────────

static std::string http_200(const std::string& body) {
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type\r\n"
           "Content-Length: " + std::to_string(body.size()) + "\r\n"
           "\r\n" + body;
}

static std::string http_404() {
    std::string body = "{\"error\":\"not found\"}";
    return "HTTP/1.1 404 Not Found\r\n"
           "Content-Type: application/json\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Content-Length: " + std::to_string(body.size()) + "\r\n"
           "\r\n" + body;
}

static std::string http_options() {
    return "HTTP/1.1 204 No Content\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
           "Access-Control-Allow-Headers: Content-Type\r\n"
           "\r\n";
}

// ─── JSON escape ─────────────────────────────────────────────────────────────

static std::string json_escape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

// ─── Request parser ──────────────────────────────────────────────────────────

struct Request {
    std::string method;
    std::string path;
    std::string body;
};

static Request parse_request(const std::string& raw) {
    Request req;
    std::istringstream ss(raw);
    ss >> req.method >> req.path;

    // Find body after \r\n\r\n
    auto pos = raw.find("\r\n\r\n");
    if (pos != std::string::npos)
        req.body = raw.substr(pos + 4);

    return req;
}

// Extract value for a JSON key (simple, no nested support needed)
// e.g. {"cmd":"SET foo bar"} → SET foo bar
static std::string extract_json_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find("\"", pos + search.size() + 1);
    if (pos == std::string::npos) return "";
    auto end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

// ─── Route handlers ──────────────────────────────────────────────────────────

static std::string handle_stats(KVStore& store) {
    std::string body = "{\"keys\":" + std::to_string(store.size()) + "}";
    return http_200(body);
}

static std::string handle_command(const std::string& body, KVStore& store) {
    std::string cmd = extract_json_string(body, "cmd");
    if (cmd.empty())
        return http_200("{\"error\":\"missing cmd field\"}");

    std::string response = CommandParser::dispatch(cmd, store);

    // Clean up response for JSON
    std::string cleaned = json_escape(response);
    std::string json = "{\"response\":\"" + cleaned + "\"}";
    return http_200(json);
}

// ─── Per-client handler ──────────────────────────────────────────────────────

static void handle_http_client(int client_fd, KVStore& store) {
    char buf[8192];
    memset(buf, 0, sizeof(buf));
    recv(client_fd, buf, sizeof(buf) - 1, 0);

    std::string raw(buf);
    Request req = parse_request(raw);

    std::string response;

    if (req.method == "OPTIONS") {
        response = http_options();
    } else if (req.method == "GET" && req.path == "/stats") {
        response = handle_stats(store);
    } else if (req.method == "POST" && req.path == "/cmd") {
        response = handle_command(req.body, store);
    } else {
        response = http_404();
    }

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}

// ─── HTTP server loop ────────────────────────────────────────────────────────

void run_http_server(KVStore& store, int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { std::cerr << "[http] socket() failed\n"; return; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[http] bind() failed on port " << port << "\n";
        close(server_fd); return;
    }
    listen(server_fd, 64);
    std::cout << "[http-api] listening on port " << port << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;
        std::thread(handle_http_client, client_fd, std::ref(store)).detach();
    }
    close(server_fd);
}
