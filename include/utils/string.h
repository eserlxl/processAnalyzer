// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#ifndef UTILS_STRING_H
#define UTILS_STRING_H

#include "utils/types.h"
#include <string>
#include <string_view>
#include <vector>
#include <format> // For std::format_string and std::vformat
#include <charconv> // For std::from_chars
#include <cmath> // Include for std::isfinite

namespace utils {

/**
 * @brief Safely parses a string view and updates the output variable on success.
 *
 * This function attempts to parse the string view `s` into a value of type `T`.
 * If parsing is successful, the `out` variable is updated with the parsed value.
 * If parsing fails, `out` remains unchanged. This function is useful for parsing
 * multiple values from a string without complex error handling for each one.
 * It uses `std::from_chars` for locale-independent parsing.
 *
 * @tparam T The numeric type to parse into (e.g., int, long, double).
 * @param s The string view to parse.
 * @param out A reference to the variable that will receive the parsed value.
 */
template<typename T>
[[nodiscard]] inline bool tryParse(std::string_view s, T& out) {
    constexpr std::string_view whitespace = " \t\n\r\f\v";
    auto firstCharPos = s.find_first_not_of(whitespace);
    if (firstCharPos == std::string_view::npos) {
        return false;
    }
    auto lastCharPos = s.find_last_not_of(whitespace);
    std::string_view trimmedSv = s.substr(firstCharPos, lastCharPos - firstCharPos + 1);

    const char* begin = trimmedSv.data();
    const char* end = begin + trimmedSv.size();

    if (!trimmedSv.empty() && trimmedSv.front() == '+') {
        begin++;
    }

    T tempVal{};
    auto [ptr, ec] = std::from_chars(begin, end, tempVal);

    if (ec == std::errc{} && ptr == end) {
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(tempVal)) {
                return false;
            }
        }
        out = tempVal;
        return true;
    }
    return false;
}

template <typename TInt>
[[nodiscard]] inline std::optional<TInt> parseIntegerNoThrow(std::string_view text, int base = 10) {
    TInt value{};
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value, base);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

inline constexpr int defaultRadix = 10;

/**
 * @brief Provides utility functions for string manipulation and parsing.
 *
 * This namespace contains various functions to perform common operations on strings,
 * including trimming, case conversion, searching, replacing, splitting, joining,
 * and numeric parsing.
 */

// String Manipulation

/**
 * @brief Removes leading and trailing whitespace from a string view.
 * @param s The string view to trim.
 * @return A new string with leading and trailing whitespace removed.
 */
[[nodiscard]] std::string trim(std::string_view s);

/**
 * @brief Checks if a string view starts with a specified prefix.
 * @param s The string view to check.
 * @param prefix The prefix to look for.
 * @return True if the string starts with the prefix, false otherwise.
 */
[[nodiscard]] bool startsWith(std::string_view s, std::string_view prefix);

/**
 * @brief Checks if a string view ends with a specified suffix.
 * @param s The string view to check.
 * @param suffix The suffix to look for.
 * @return True if the string ends with the suffix, false otherwise.
 */
[[nodiscard]] bool endsWith(std::string_view s, std::string_view suffix);

/**
 * @brief Checks if a string view contains a specified substring.
 * @param s The string view to search within.
 * @param substring The substring to look for.
 * @return True if the string contains the substring, false otherwise.
 */
[[nodiscard]] bool contains(std::string_view s, std::string_view substring);

/**
 * @brief Checks if a string view starts with a specified prefix, ignoring case.
 *
 * This function performs a locale-independent, case-insensitive comparison,
 * only considering ASCII characters 'A'-'Z' and 'a'-'z' for case conversion.
 * @param str The string view to check.
 * @param prefix The prefix to look for.
 * @return True if the string starts with the prefix (case-insensitive), false otherwise.
 */
[[nodiscard]] bool startsWithIgnoreCase(std::string_view str, std::string_view prefix);

/**
 * @brief Checks if a string view ends with a specified suffix, ignoring case.
 *
 * This function performs a locale-independent, case-insensitive comparison,
 * only considering ASCII characters 'A'-'Z' and 'a'-'z' for case conversion.
 * @param str The string view to check.
 * @param suffix The suffix to look for.
 * @return True if the string ends with the suffix (case-insensitive), false otherwise.
 */
[[nodiscard]] bool endsWithIgnoreCase(std::string_view str, std::string_view suffix);

/**
 * @brief Checks if a string view contains a specified substring, ignoring case.
 *
 * This function performs a locale-independent, case-insensitive comparison,
 * only considering ASCII characters 'A'-'Z' and 'a'-'z' for case conversion.
 * @param str The string view to search within.
 * @param subStr The substring to look for.
 * @return True if the string contains the substring (case-insensitive), false otherwise.
 */
[[nodiscard]] bool containsIgnoreCase(std::string_view str, std::string_view subStr);

/**
 * @brief Converts a string view to its lowercase equivalent.
 *
 * This function performs a locale-independent conversion to lowercase.
 * Only ASCII characters 'A'-'Z' are converted to 'a'-'z'. Other characters remain unchanged.
 * @param s The string view to convert.
 * @return A new string with all characters converted to lowercase.
 */
[[nodiscard]] std::string toLower(std::string_view s);

/**
 * @brief Converts a string view to its uppercase equivalent.
 *
 * This function performs a locale-independent conversion to uppercase.
 * Only ASCII characters 'a'-'z' are converted to 'A'-'Z'. Other characters remain unchanged.
 * @param s The string view to convert.
 * @return A new string with all characters converted to uppercase.
 */
[[nodiscard]] std::string toUpper(std::string_view s);

/**
 * @brief Replaces all occurrences of a target substring with a replacement substring in a string view.
 * @param s The string view to perform replacements in.
 * @param target The substring to search for.
 * @param replacement The substring to replace `target` with.
 * @return A new string with all occurrences replaced.
 */
[[nodiscard]] std::string replaceAll(std::string_view s, std::string_view target, std::string_view replacement);

/**
 * @brief Replaces the first occurrence of a target substring with a replacement substring in a string view.
 * @param s The string view to perform replacement in.
 * @param from The substring to search for.
 * @param to The substring to replace `from` with.
 * @return A new string with the first occurrence replaced.
 */
[[nodiscard]] std::string replaceFirst(std::string_view s, std::string_view from, std::string_view to);

/**
 * @brief Replaces up to 'count' occurrences of a target substring with a replacement substring in a string view.
 * @param s The string view to perform replacements in.
 * @param from The substring to search for.
 * @param to The substring to replace `from` with.
 * @param count The maximum number of occurrences to replace.
 * @return A new string with up to 'count' occurrences replaced.
 */
[[nodiscard]] std::string replaceN(std::string_view s, std::string_view from, std::string_view to, size_t count);

/**
 * @brief Joins a vector of strings into a single string using a specified delimiter.
 * @param parts A constant reference to a vector of strings to join.
 * @param delimiter The string view to use as a separator between parts.
 * @return A single string formed by joining the parts with the delimiter.
 */
[[nodiscard]] std::string join(const std::vector<std::string>& parts, std::string_view delimiter);

/**
 * @brief Formats a string using a format string and arguments.
 *
 * This function leverages C++23's std::format_string for compile-time format string validation,
 * ensuring type safety and catching format errors at compile time.
 * @tparam Args Variadic template arguments for formatting.
 * @param fmt The format string. Must be a compile-time constant for validation.
 * @param args The arguments to format.
 * @return The formatted string.
 */
template<typename... Args>
[[nodiscard]] std::string format(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}

/**
 * @brief Splits a string view into a vector of strings based on a character delimiter.
 * @param s The string view to split.
 * @param delimiter The character to split the string by.
 * @param skipEmpty If true, empty parts resulting from consecutive delimiters are skipped.
 * @return A vector of strings representing the split parts.
 */
[[nodiscard]] std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false);

/**
 * @brief Splits a string view into a vector of strings based on a string view delimiter.
 * @param s The string view to split.
 * @param delimiter The string view to split the string by.
 * @param skipEmpty If true, empty parts resulting from consecutive delimiters are skipped.
 * @return A vector of strings representing the split parts.
 */
[[nodiscard]] std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false);

// Numeric Parsing/Validation

/**
 * @brief Checks if a string view represents a valid integer.
 * @param s The string view to check.
 * @return True if the string contains a valid integer, false otherwise.
 */
[[nodiscard]] bool isInteger(std::string_view s);

/**
 * @brief Checks if a string view represents a valid floating-point number.
 * @param s The string view to check.
 * @return True if the string contains a valid floating-point number, false otherwise.
 */
[[nodiscard]] bool isFloatingPoint(std::string_view s);

/**
 * @brief Converts a string view to a long integer.
 *
 * This function uses `std::from_chars` for robust, locale-independent parsing.
 * It returns a `Result<long>` indicating success or failure and containing the parsed value.
 * @param s The string view to convert.
 * @param base The numeric base to use (e.g., 10 for decimal, 16 for hexadecimal).
 * @return A `Result<long>` containing the converted value or an error.
 */
[[nodiscard]] Result<long> toLong(std::string_view s, int base = defaultRadix);

/**
 * @brief Converts a string view to a double-precision floating-point number.
 *
 * This function uses `std::from_chars` for robust, locale-independent parsing.
 * It returns a `Result<double>` indicating success or failure and containing the parsed value.
 * @param s The string view to convert.
 * @return A `Result<double>` containing the converted value or an error.
 */
[[nodiscard]] Result<double> toDouble(std::string_view s);

/**
 * @brief Parses a string view into a boolean value.
 *
 * Recognizes "true", "false", "1", and "0" (case-insensitive for "true"/"false").
 * Case-insensitive comparison is performed using locale-independent ASCII-only conversion.
 * Returns a `Result<bool>` indicating success or failure.
 * @param s The string view to parse.
 * @return A `Result<bool>` containing the parsed boolean value or an error.
 */
[[nodiscard]] Result<bool> parseBool(std::string_view s);

/**
 * @brief Converts a string view to an integer.
 *
 * This function uses `std::from_chars` for robust, locale-independent parsing.
 * It returns a `Result<int>` indicating success or failure and containing the parsed value.
 * @param s The string view to convert.
 * @param base The numeric base to use (e.g., 10 for decimal, 16 for hexadecimal).
 * @return A `Result<int>` containing the converted value or an error.
 */
[[nodiscard]] Result<int> toInt(std::string_view s, int base = defaultRadix);

/**
 * @brief Converts a string view to a single-precision floating-point number.
 *
 * This function uses `std::from_chars` for robust, locale-independent parsing.
 * It returns a `Result<float>` indicating success or failure and containing the parsed value.
 * @param s The string view to convert.
 * @return A `Result<float>` containing the converted value or an error.
 */
[[nodiscard]] Result<float> toFloat(std::string_view s);

} // namespace utils

#endif // UTILS_STRING_H
