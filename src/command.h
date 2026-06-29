#pragma once
#include <string>
#include "store.h"

namespace CommandParser {
    std::string dispatch(const std::string& raw, KVStore& store);
}
