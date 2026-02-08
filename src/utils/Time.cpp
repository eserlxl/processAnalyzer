// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/Time.h"
#include <ctime>
#include <array>
#include <iomanip>
#include <sstream>

namespace utils {

std::string formatElapsedTime(long long seconds) {
    if (seconds < 0) return "N/A";
    
    constexpr long long kSecondsPerMinute = 60;
    constexpr long long kSecondsPerHour = 3600;
    constexpr long long kSecondsPerDay = 24LL * 3600LL;

    long long days = seconds / kSecondsPerDay;
    seconds %= kSecondsPerDay;
    long long hours = seconds / kSecondsPerHour;
    seconds %= kSecondsPerHour;
    long long minutes = seconds / kSecondsPerMinute;
    seconds %= kSecondsPerMinute;

    std::string result;
    if (days > 0) {
        result += std::to_string(days) + "d ";
    }
    // Always show H:M:S, even if days are present. Pad with leading zeros.
    std::string h = std::to_string(hours);
    std::string m = std::to_string(minutes);
    std::string s = std::to_string(seconds);
    if (h.length() == 1) h = "0" + h;
    if (m.length() == 1) m = "0" + m;
    if (s.length() == 1) s = "0" + s;

    result += h + ":" + m + ":" + s;
    return result;
}

std::string formatTimestamp(long long unixTimestamp) {
    if (unixTimestamp <= 0) { // Handle invalid or epoch timestamps gracefully
        return "N/A";
    }

    // Use standard C time functions for portability
    auto tt = static_cast<std::time_t>(unixTimestamp);
    std::tm tmBuf{};
    
    // Use localtime_r for thread safety if available, or localtime
#if defined(_POSIX_C_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE)
    localtime_r(&tt, &tmBuf);
#else
    // Fallback for non-POSIX
    if (std::tm* tmp = std::localtime(&tt)) {
        tmBuf = *tmp;
    }
#endif

    constexpr size_t kBufferSize = 64;
    std::array<char, kBufferSize> buffer{};
    // %Z or %z for timezone
    if (std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S %Z", &tmBuf)) {
        return {buffer.data()};
    }
    return "N/A";
}

Result<std::chrono::system_clock::time_point> getCurrentSystemTime() {
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::chrono::steady_clock::time_point> getCurrentSteadyTime() {
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::string> formatTimestamp(std::chrono::system_clock::time_point tp, std::string_view formatStr) {
    (void)tp;
    (void)formatStr;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::chrono::system_clock::time_point> parseTimestamp(std::string_view timestampStr, std::string_view formatStr) {
    (void)timestampStr;
    (void)formatStr;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

} // namespace utils
