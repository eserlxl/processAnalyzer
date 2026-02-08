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

    constexpr long long kSecondsPerMinute = 60;
    constexpr long long kSecondsPerHour = 3600;
    constexpr long long kSecondsPerDay = 24LL * 3600LL;

    long long days = seconds / kSecondsPerDay;
    seconds %= kSecondsPerDay;
    long long hours = seconds / kSecondsPerHour;
    seconds %= kSecondsPerHour;
    long long minutes = seconds / kSecondsPerMinute;
    seconds %= kSecondsPerMinute;

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
    
#if defined(_POSIX_C_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE)
    ::localtime_r(&tt, &tmBuf);
#else
    if (::std::tm* tmp = ::std::localtime(&tt)) {
        tmBuf = *tmp;
    }
#endif

    constexpr size_t kBufferSize = 64;
    ::std::array<char, kBufferSize> buffer{};
    if (::std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &tmBuf)) {
        return {buffer.data()};
    }
    return "N/A";
}

Result<::std::chrono::system_clock::time_point> getCurrentSystemTime() {
    return ::std::chrono::system_clock::now();
}

Result<::std::chrono::steady_clock::time_point> getCurrentSteadyTime() {
    return ::std::chrono::steady_clock::now();
}

Result<::std::string> formatTimestamp(::std::chrono::system_clock::time_point tp, ::std::string_view formatStr) {
    auto tt = ::std::chrono::system_clock::to_time_t(tp);
    ::std::tm tmBuf{};

#if defined(_POSIX_C_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE)
    ::localtime_r(&tt, &tmBuf);
#else
    if (::std::tm* tmp = ::std::localtime(&tt)) {
        tmBuf = *tmp;
    } else {
        return ::std::unexpected(make_error_code(UtilsError::unknownError));
    }
#endif

    ::std::stringstream ss;
    ss << ::std::put_time(&tmBuf, formatStr.data());
    if (ss.fail()) {
         return ::std::unexpected(make_error_code(UtilsError::unknownError));
    }
    return ss.str();
}

Result<::std::chrono::system_clock::time_point> parseTimestamp(::std::string_view timestampStr, ::std::string_view formatStr) {
    ::std::tm tmBuf{};
    ::std::stringstream ss;
    ss << timestampStr;
    ss >> ::std::get_time(&tmBuf, formatStr.data());
    
    if (ss.fail()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // Set fields not parsed by get_time to valid defaults if possible, or rely on mktime
    tmBuf.tm_isdst = -1; // Let mktime determine DST
    
    ::std::time_t tt = ::std::mktime(&tmBuf);
    if (tt == -1) {
         return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    
    return ::std::chrono::system_clock::from_time_t(tt);
}

} // namespace utils
