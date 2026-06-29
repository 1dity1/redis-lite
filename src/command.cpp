#include "command.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

static std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string cur;
    bool in_quotes = false;
    for (char c : input) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if ((c == ' ' || c == '\t' || c == '\r' || c == '\n') && !in_quotes) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

static std::string ok()                        { return "+OK\r\n"; }
static std::string pong()                      { return "+PONG\r\n"; }
static std::string bye()                       { return "+BYE\r\n"; }
static std::string nil()                       { return "$-1\r\n"; }
static std::string integer(int n)              { return ":" + std::to_string(n) + "\r\n"; }
static std::string integer64(long long n)      { return ":" + std::to_string(n) + "\r\n"; }
static std::string bulk(const std::string& s)  { return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n"; }
static std::string err(const std::string& msg) { return "-ERR " + msg + "\r\n"; }

std::string CommandParser::dispatch(const std::string& raw, KVStore& store) {
    auto tokens = tokenize(raw);
    if (tokens.empty()) return err("empty command");

    std::string cmd = to_upper(tokens[0]);

    if (cmd == "PING")   return pong();
    if (cmd == "QUIT")   return bye();
    if (cmd == "DBSIZE") return integer(static_cast<int>(store.size()));

    if (cmd == "SET") {
        if (tokens.size() < 3) return err("SET requires key and value");
        store.set(tokens[1], tokens[2]);
        return ok();
    }
    if (cmd == "SETEX") {
        if (tokens.size() < 4) return err("SETEX requires key, seconds, value");
        int secs = 0;
        try { secs = std::stoi(tokens[2]); }
        catch (...) { return err("seconds must be an integer"); }
        if (secs <= 0) return err("seconds must be positive");
        store.set_ex(tokens[1], tokens[3], secs);
        return ok();
    }
    if (cmd == "GET") {
        if (tokens.size() < 2) return err("GET requires key");
        auto val = store.get(tokens[1]);
        if (!val) return nil();
        return bulk(*val);
    }
    if (cmd == "DEL") {
        if (tokens.size() < 2) return err("DEL requires key");
        return integer(store.del(tokens[1]) ? 1 : 0);
    }
    if (cmd == "EXISTS") {
        if (tokens.size() < 2) return err("EXISTS requires key");
        return integer(store.exists(tokens[1]) ? 1 : 0);
    }
    if (cmd == "TTL") {
        if (tokens.size() < 2) return err("TTL requires key");
        return integer(store.ttl(tokens[1]));
    }

    // INCR key — increment by 1
    if (cmd == "INCR") {
        if (tokens.size() < 2) return err("INCR requires key");
        auto [ok, val] = store.incr_by(tokens[1], 1);
        if (!ok) return err("value is not an integer");
        return integer64(val);
    }

    // DECR key — decrement by 1
    if (cmd == "DECR") {
        if (tokens.size() < 2) return err("DECR requires key");
        auto [ok, val] = store.incr_by(tokens[1], -1);
        if (!ok) return err("value is not an integer");
        return integer64(val);
    }

    // INCRBY key delta
    if (cmd == "INCRBY") {
        if (tokens.size() < 3) return err("INCRBY requires key and delta");
        long long delta = 0;
        try { delta = std::stoll(tokens[2]); }
        catch (...) { return err("delta must be an integer"); }
        auto [ok, val] = store.incr_by(tokens[1], delta);
        if (!ok) return err("value is not an integer");
        return integer64(val);
    }

    // DECRBY key delta
    if (cmd == "DECRBY") {
        if (tokens.size() < 3) return err("DECRBY requires key and delta");
        long long delta = 0;
        try { delta = std::stoll(tokens[2]); }
        catch (...) { return err("delta must be an integer"); }
        auto [ok, val] = store.incr_by(tokens[1], -delta);
        if (!ok) return err("value is not an integer");
        return integer64(val);
    }

    return err("unknown command '" + tokens[0] + "'");
}
