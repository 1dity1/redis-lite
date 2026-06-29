#pragma once
#include <string>
#include "../src/store.h"
#include "../src/command.h"

// Starts HTTP API on port 6380
// Runs in its own thread — non-blocking for main server
void run_http_server(KVStore& store, int port = 6380);
