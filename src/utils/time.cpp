// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/time.h"
#include <ctime>
#include <array>
#include <iomanip>
#include <sstream>
#include <chrono> // Ensure chrono is included for time_point

namespace utils {

// Define constants for clarity and maintainability, ensuring long long arithmetic
constexpr long long secondsPerMinute = 60LL;
constexpr long long secondsPerHour = 60LL * secondsPerMinute;
constexpr long long secondsPerDay = 24LL * secondsPerHour;

/// @brief Formats a duration in seconds into a human-readable string (e.g., "1d 2h 3m 4s").
///
/// @param seconds The total number of seconds.
/// @return A string representing the formatted duration, or an error code if the input is negative.
Result<::std::string> formatElapsedTime(long long seconds) {
    if (seconds < 0) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

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
    if (hours > 0 || days > 0) { // Show hours if non-zero or if days are present
        result += ::std::to_string(hours) + "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) { // Show minutes if non-zero or if hours/days are present
        result += ::std::to_string(minutes) + "m ";
    }
    result += ::std::to_string(seconds) + "s"; // Always show seconds
    return result;
}

/// @brief Formats a Unix timestamp (seconds since epoch) into a human-readable string (YYYY-MM-DD HH:MM:SS).
///
/// WARNING: This function relies on std::time_t, which may be a 32-bit integer
/// on some systems, leading to the "Year 2038 problem". Timestamps beyond
/// January 19, 2038, may not be represented correctly. For wider compatibility,
/// consider using C++20 chrono features or a dedicated time library.
///
/// @param unixTimestamp The Unix timestamp in seconds.
/// @return A string representing the formatted date and time (local time), or an error code.
Result<::std::string> formatTimestamp(long long unixTimestamp) {
    if (unixTimestamp < 0) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    auto tt = static_cast<::std::time_t>(unixTimestamp);
    ::std::tm tmBuf{};
    bool success = false;

    // Use thread-safe C runtime functions for time conversion
#ifdef _WIN32
    if (::localtime_s(&tmBuf, &tt) == 0) {
        success = true;
    }
#elif defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__)
    // Use standard POSIX localtime_r.
    if (localtime_r(&tt, &tmBuf) != nullptr) {
        success = true;
    }
#else
    // Fallback for other systems, less thread-safe.
    if (::std::tm* tmp = ::std::localtime(&tt)) {
        tmBuf = *tmp;
        success = true;
    }
#endif

    if (!success) {
        return ::std::unexpected(make_error_code(UtilsError::unknownError));
    }

    // Use std::array for buffer for safety
    constexpr size_t bufferSize = 64; // Sufficient for YYYY-MM-DD HH:MM:SS and null terminator
    ::std::array<char, bufferSize> buffer{};
    if (::strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &tmBuf)) {
        return ::std::string(buffer.data());
    }
    // If strftime fails
    return ::std::unexpected(make_error_code(UtilsError::unknownError));
}

/// @brief Gets the current time from the system clock.
/// @return A time_point from system_clock, or an error code.
Result<::std::chrono::system_clock::time_point> getCurrentSystemTime() {
    return ::std::chrono::system_clock::now();
}

/// @brief Gets the current time from the steady clock.
/// @return A time_point from steady_clock, or an error code.
Result<::std::chrono::steady_clock::time_point> getCurrentSteadyTime() {
    return ::std::chrono::steady_clock::now();
}

/// @brief Formats a system_clock::time_point into a string using a specified format.
///
/// This function interprets the time_point in the system's local timezone.
///
/// @param tp The time_point to format.
/// @param formatStr The format string (e.g., "%Y-%m-%d %H:%M:%S").
/// @return A formatted string, or an error code if the format string is empty or formatting fails.
Result<::std::string> formatTimestamp(::std::chrono::system_clock::time_point tp, const ::std::string& formatStr) {
    if (formatStr.empty()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    auto tt = ::std::chrono::system_clock::to_time_t(tp);
    ::std::tm tmBuf{};
    bool success = false;

    // Use thread-safe C runtime functions for time conversion
#ifdef _WIN32
    if (::localtime_s(&tmBuf, &tt) == 0) {
        success = true;
    }
#elif defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__)
    // Use standard POSIX localtime_r.
    if (localtime_r(&tt, &tmBuf) != nullptr) {
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
    // Use std::put_time for formatting
    ss << ::std::put_time(&tmBuf, formatStr.c_str());
    if (ss.fail() || ss.str().empty()) {
         return ::std::unexpected(make_error_code(UtilsError::unknownError));
    }
    return ss.str();
}

/// @brief Parses a timestamp string into a system_clock::time_point.
///
/// This function interprets the input timestamp string according to the provided
/// format string and the system's local timezone. The `std::mktime` function
/// is used, which assumes local time for the input tm struct and converts it
/// to a UTC std::time_t. This can lead to ambiguity if the input is intended
/// to be UTC.
///
/// WARNING: This function relies on std::time_t for conversion, which may be a
/// 32-bit integer on some systems, leading to the "Year 2038 problem". Timestamps
/// beyond January 19, 2038, may not be represented correctly.
///
/// @param timestampStr The string to parse.
/// @param formatStr The format string (e.g., "%Y-%m-%d %H:%M:%S").
/// @return A std::chrono::system_clock::time_point, or an error code if parsing fails or the date is invalid.
Result<::std::chrono::system_clock::time_point> parseTimestamp(const ::std::string& timestampStr, const ::std::string& formatStr) {
    if (timestampStr.empty() || formatStr.empty()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    ::std::tm tmBuf{};
    ::std::stringstream ss;
    ss << timestampStr;
    // Use std::get_time for parsing
    ss >> ::std::get_time(&tmBuf, formatStr.c_str());
    
    if (ss.fail()) {
        // Parsing failed, return error
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // Ensure mktime correctly handles time zones and DST.
    // tm_isdst = -1 tells mktime to determine if DST is in effect.
    tmBuf.tm_isdst = -1; 
    
    ::std::time_t tt = ::std::mktime(&tmBuf);
    if (tt == -1) {
         // mktime failed, likely due to an invalid date/time combination that it couldn't normalize.
         return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // Round-trip check: Convert the time_t back to tm, format it, and compare with the original string.
    // This is crucial for detecting invalid dates like Feb 30th that mktime might normalize.
    ::std::tm tmBufCheck{};
    bool success = false;
#ifdef _WIN32
    if (::localtime_s(&tmBufCheck, &tt) == 0) {
        success = true;
    }
#elif defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__)
    // Use standard POSIX localtime_r.
    if (localtime_r(&tt, &tmBufCheck) != nullptr) {
        success = true;
    }
#else
    if (::std::tm* tmp = ::std::localtime(&tt)) {
        tmBufCheck = *tmp;
        success = true;
    }
#endif

    if (!success) {
        return ::std::unexpected(make_error_code(UtilsError::unknownError)); // Should not happen if mktime succeeded
    }

    ::std::stringstream checkSs;
    checkSs << ::std::put_time(&tmBufCheck, formatStr.c_str());
    
    // Compare the formatted string with the original input string.
    // If they don't match, it means mktime normalized an invalid date/time.
    if (checkSs.fail() || checkSs.str() != timestampStr) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    
    return ::std::chrono::system_clock::from_time_t(tt);
}

} // namespace utils
