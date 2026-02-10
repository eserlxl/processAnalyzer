// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/Time.h"
#include <ctime>
#include <array>
#include <iomanip>
#include <sstream>

namespace utils {

::std::string formatElapsedTime(long long seconds) {
    if (seconds < 0) return "N/A";

    constexpr long long secondsPerMinute = 60;
    constexpr long long secondsPerHour = 3600;
    constexpr long long secondsPerDay = 24LL * 3600LL;

    long long days = seconds / secondsPerDay;
    seconds %= secondsPerDay;
    long long hours = seconds / secondsPerHour;
    seconds %= secondsPerHour;
    long long minutes = seconds / secondsPerMinute;
    seconds %= secondsPerMinute;

    ::std::string result;
    if (days > 0) {
        result += ::std::to_string(days) + "d ";
    }
    if (hours > 0 || days > 0) {
        result += ::std::to_string(hours) + "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        result += ::std::to_string(minutes) + "m ";
    }
    result += ::std::to_string(seconds) + "s";
    return result;
}

::std::string formatTimestamp(long long unixTimestamp) {
    if (unixTimestamp < 0) {
        return "N/A";
    }

    auto tt = static_cast<::std::time_t>(unixTimestamp);
    ::std::tm tmBuf{};
    bool success = false;

#ifdef _WIN32
    // Use localtime_s for Windows for thread-safety
    if (::localtime_s(&tmBuf, &tt) == 0) {
        success = true;
    }
#elif defined(_POSIX_C_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE)
    // Use localtime_r for POSIX systems for thread-safety
    if (::localtime_r(&tt, &tmBuf) != nullptr) {
        success = true;
    }
#else
    // Fallback for other systems, though less thread-safe.
    // We still check if std::localtime returns a valid pointer.
    if (::std::tm* tmp = ::std::localtime(&tt)) {
        tmBuf = *tmp;
        success = true;
    }
#endif

    if (!success) {
        return "N/A"; // Explicitly return N/A on failure
    }

    constexpr size_t bufferSize = 64;
    ::std::array<char, bufferSize> buffer{};
    if (::std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &tmBuf)) {
        return {buffer.data()};
    }
    // If strftime fails for any reason after localtime succeeded
    return "N/A";
}

Result<::std::chrono::system_clock::time_point> getCurrentSystemTime() {
    return ::std::chrono::system_clock::now();
}

Result<::std::chrono::steady_clock::time_point> getCurrentSteadyTime() {
    return ::std::chrono::steady_clock::now();
}

Result<::std::string> formatTimestamp(::std::chrono::system_clock::time_point tp, const ::std::string& formatStr) {
    if (formatStr.empty()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    auto tt = ::std::chrono::system_clock::to_time_t(tp);
    ::std::tm tmBuf{};
    bool success = false;

#ifdef _WIN32
    if (::localtime_s(&tmBuf, &tt) == 0) {
        success = true;
    }
#elif defined(_POSIX_C_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE)
    if (::localtime_r(&tt, &tmBuf) != nullptr) {
        success = true;
    }
#else
    if (::std::tm* tmp = ::std::localtime(&tt)) {
        tmBuf = *tmp;
        success = true;
    }
#endif

    if (!success) {
        return ::std::unexpected(make_error_code(UtilsError::unknownError));
    }

    ::std::stringstream ss;
    ss << ::std::put_time(&tmBuf, formatStr.c_str());
    if (ss.fail() || ss.str().empty()) {
         return ::std::unexpected(make_error_code(UtilsError::unknownError));
    }
    return ss.str();
}

Result<::std::chrono::system_clock::time_point> parseTimestamp(const ::std::string& timestampStr, const ::std::string& formatStr) {
    if (timestampStr.empty() || formatStr.empty()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    ::std::tm tmBuf{};
    ::std::stringstream ss;
    ss << timestampStr;
    ss >> ::std::get_time(&tmBuf, formatStr.c_str());
    
    if (ss.fail()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // Set fields not parsed by get_time to valid defaults if possible, or rely on mktime
    tmBuf.tm_isdst = -1; // Let mktime determine DST
    
    ::std::time_t tt = ::std::mktime(&tmBuf);
    if (tt == -1) {
         return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // After mktime, re-format tmBuf to check if it matches the original timestampStr.
    // This detects cases where mktime normalizes invalid dates (e.g., Feb 30th).
    ::std::stringstream checkSs;
    checkSs << ::std::put_time(&tmBuf, formatStr.c_str());
    if (checkSs.fail() || checkSs.str() != timestampStr) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    
    return ::std::chrono::system_clock::from_time_t(tt);
}

} // namespace utils
