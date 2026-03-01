// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_TIME_H
#define UTILS_TIME_H

#include "utils/types.h"
#include <string>
#include <string_view>
#include <chrono>

namespace utils {

    // Time Utilities
    /// @brief Gets the current system time.
    ///
    /// Retrieves the current time from the system clock. This time is generally not monotonic
    /// and can be affected by system time adjustments.
    ///
    /// @return A Result object containing the current system clock time_point on success,
    ///         or an error if the time cannot be retrieved.
    Result<std::chrono::system_clock::time_point> getCurrentSystemTime();
    /// @brief Gets the current steady time.
    ///
    /// Retrieves the current time from the steady clock. This clock is monotonic and suitable
    /// for measuring durations, unaffected by system time changes.
    ///
    /// @return A Result object containing the current steady clock time_point on success,
    ///         or an error if the time cannot be retrieved.
    Result<std::chrono::steady_clock::time_point> getCurrentSteadyTime();
    /// @brief Formats a time_point into a string according to a specified format.
    ///
    /// This function takes a `std::chrono::system_clock::time_point` and a format string
    /// to produce a human-readable date and time representation. It is designed to be
    /// thread-safe and handles potential errors related to the format string.
    ///
    /// @param tp The time_point to format.
    /// @param formatStr The format string (e.g., "%Y-%m-%d %H:%M:%S"). Refer to `strftime` for valid specifiers.
    /// @return A Result object containing the formatted string on success, or an error if the format string is invalid or formatting fails.
    Result<std::string> formatTimestamp(std::chrono::system_clock::time_point tp, const std::string& formatStr);
    /// @brief Formats a Unix timestamp into a string.
    ///
    /// @param unixTimestamp The Unix timestamp in seconds since epoch.
    /// @return A Result object containing the formatted string on success, or an error.
    Result<std::string> formatTimestamp(long long unixTimestamp);
    /// @brief Parses a timestamp string into a time_point according to a specified format.
    ///
    /// Converts a string representation of a timestamp into a `std::chrono::system_clock::time_point`.
    /// This function is thread-safe and robust against malformed input strings.
    ///
    /// @param timestampStr The string to parse (e.g., "2023-10-27 10:30:00").
    /// @param formatStr The format string that describes the `timestampStr` (e.g., "%Y-%m-%d %H:%M:%S"). Refer to `strptime` for valid specifiers.
    /// @return A Result object containing the parsed time_point on success, or an error if the string does not match the format or parsing fails.
    Result<std::chrono::system_clock::time_point> parseTimestamp(const std::string& timestampStr, const std::string& formatStr);
    /// @brief Formats a duration in seconds into a human-readable string (e.g., "X days, Y hours, Z minutes, W seconds").
    ///
    /// This function converts a raw number of seconds into a more understandable elapsed time format.
    /// It handles potential edge cases for the input 'seconds' value and returns a Result object.
    ///
    /// @param seconds The total number of seconds to format. Expected to be within a reasonable range for `long long`.
    /// @return A Result object containing the formatted string on success, or an error if formatting fails.
    Result<std::string> formatElapsedTime(long long seconds);

} // namespace utils

#endif // UTILS_TIME_H
