// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_TIME_H
#define UTILS_TIME_H

#include "utils/Types.h"
#include <string>
#include <string_view>
#include <chrono>

namespace utils {

// Time Utilities
Result<std::chrono::system_clock::time_point> getCurrentSystemTime();
Result<std::chrono::steady_clock::time_point> getCurrentSteadyTime();
Result<std::string> formatTimestamp(std::chrono::system_clock::time_point tp, std::string_view formatStr);
Result<std::chrono::system_clock::time_point> parseTimestamp(std::string_view timestampStr, std::string_view formatStr);
std::string formatElapsedTime(long long seconds);
std::string formatTimestamp(long long unixTimestamp);

} // namespace utils

#endif // UTILS_TIME_H
