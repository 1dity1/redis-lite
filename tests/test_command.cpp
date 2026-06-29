#include <gtest/gtest.h>
#include "command.h"

struct CmdTest : ::testing::Test {
    KVStore store{10};
    std::string run(const std::string& cmd) {
        return CommandParser::dispatch(cmd, store);
    }
};

TEST_F(CmdTest, Ping)           { EXPECT_EQ(run("PING"), "+PONG\r\n"); }
TEST_F(CmdTest, Quit)           { EXPECT_EQ(run("QUIT"), "+BYE\r\n"); }
TEST_F(CmdTest, SetGet)         { run("SET name aditya"); EXPECT_EQ(run("GET name"), "$6\r\naditya\r\n"); }
TEST_F(CmdTest, GetNil)         { EXPECT_EQ(run("GET ghost"), "$-1\r\n"); }
TEST_F(CmdTest, DelExists)      { run("SET k v"); EXPECT_EQ(run("DEL k"), ":1\r\n"); }
TEST_F(CmdTest, DelMissing)     { EXPECT_EQ(run("DEL ghost"), ":0\r\n"); }
TEST_F(CmdTest, Exists)         { run("SET k v"); EXPECT_EQ(run("EXISTS k"), ":1\r\n"); }
TEST_F(CmdTest, TTLNoExpiry)    { run("SET k v"); EXPECT_EQ(run("TTL k"), ":-1\r\n"); }
TEST_F(CmdTest, TTLMissing)     { EXPECT_EQ(run("TTL ghost"), ":-2\r\n"); }
TEST_F(CmdTest, CaseInsensitive){ EXPECT_EQ(run("ping"), "+PONG\r\n"); }
TEST_F(CmdTest, UnknownCommand) { EXPECT_EQ(run("FLUSHALL"), "-ERR unknown command 'FLUSHALL'\r\n"); }
TEST_F(CmdTest, MissingArgs)    { EXPECT_EQ(run("SET onlykey"), "-ERR SET requires key and value\r\n"); }
TEST_F(CmdTest, QuotedValue)    { run("SET msg \"hello world\""); EXPECT_EQ(run("GET msg"), "$11\r\nhello world\r\n"); }

TEST_F(CmdTest, DBSize) {
    EXPECT_EQ(run("DBSIZE"), ":0\r\n");
    run("SET a 1"); run("SET b 2");
    EXPECT_EQ(run("DBSIZE"), ":2\r\n");
}
TEST_F(CmdTest, SetexTTL) {
    EXPECT_EQ(run("SETEX token 100 abc"), "+OK\r\n");
    std::string ttl_resp = run("TTL token");
    EXPECT_TRUE(ttl_resp == ":100\r\n" || ttl_resp == ":99\r\n");
}

// INCR — fresh key starts from 0
TEST_F(CmdTest, IncrFreshKey) {
    EXPECT_EQ(run("INCR counter"), ":1\r\n");
    EXPECT_EQ(run("INCR counter"), ":2\r\n");
    EXPECT_EQ(run("INCR counter"), ":3\r\n");
}

// INCR on existing integer key
TEST_F(CmdTest, IncrExisting) {
    run("SET views 10");
    EXPECT_EQ(run("INCR views"), ":11\r\n");
}

// DECR
TEST_F(CmdTest, Decr) {
    run("SET stock 5");
    EXPECT_EQ(run("DECR stock"), ":4\r\n");
    EXPECT_EQ(run("DECR stock"), ":3\r\n");
}

// DECR below zero
TEST_F(CmdTest, DecrBelowZero) {
    run("SET k 0");
    EXPECT_EQ(run("DECR k"), ":-1\r\n");
}

// INCRBY
TEST_F(CmdTest, IncrBy) {
    run("SET score 100");
    EXPECT_EQ(run("INCRBY score 50"), ":150\r\n");
}

// DECRBY
TEST_F(CmdTest, DecrBy) {
    run("SET score 100");
    EXPECT_EQ(run("DECRBY score 30"), ":70\r\n");
}

// INCR on non-integer value — should error
TEST_F(CmdTest, IncrNonInteger) {
    run("SET k \"hello\"");
    EXPECT_EQ(run("INCR k"), "-ERR value is not an integer\r\n");
}
