// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "utils/String.h"
#include <algorithm>
#include <cctype> // For std::tolower, std::toupper (used carefully for ASCII only)
#include <charconv> // For std::from_chars
#include <limits>   // For std::numeric_limits
#include <cmath>    // For std::isinf, std::isfinite

namespace utils {

std::string trim(std::string_view s) {
    auto first = s.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string_view::npos) {
        return "";
    }
    auto last = s.find_last_not_of(" \t\n\r\f\v");
    return std::string(s.substr(first, (last - first + 1)));
}

bool startsWith(std::string_view s, std::string_view prefix) {
    return s.starts_with(prefix);
}

bool endsWith(std::string_view s, std::string_view suffix) {
    return s.ends_with(suffix);
}

bool contains(std::string_view s, std::string_view substring) {
    return s.find(substring) != std::string_view::npos;
}

bool startsWithIgnoreCase(std::string_view str, std::string_view prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), str.begin(),
                      [](unsigned char c1, unsigned char c2) {
                          return static_cast<unsigned char>(std::tolower(c1)) == static_cast<unsigned char>(std::tolower(c2));
                      });
}

bool endsWithIgnoreCase(std::string_view str, std::string_view suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
                      [](unsigned char c1, unsigned char c2) {
                          return static_cast<unsigned char>(std::tolower(c1)) == static_cast<unsigned char>(std::tolower(c2));
                      });
}

bool containsIgnoreCase(std::string_view str, std::string_view subStr) {
    // Manually implement for case-insensitive search without locale dependency
    if (subStr.empty()) return true;
    if (str.empty()) return false;

    std::string lowerStr = toLower(str);
    std::string lowerSubStr = toLower(subStr);

    return lowerStr.find(lowerSubStr) != std::string::npos;
}

std::string toLower(std::string_view s) {
    std::string result;
    result.reserve(s.length());
    for (char c : s) {
        if (c >= 'A' && c <= 'Z') {
            result += static_cast<char>(c + ('a' - 'A'));
        }
        else {
            result += c;
        }
    }
    return result;
}

std::string toUpper(std::string_view s) {
    std::string result;
    result.reserve(s.length());
    for (char c : s) {
        if (c >= 'a' && c <= 'z') {
            result += static_cast<char>(c - ('a' - 'A'));
        }
        else {
            result += c;
        }
    }
    return result;
}

std::string replaceAll(std::string_view s, std::string_view target, std::string_view replacement) {
    if (target.empty()) {
        return std::string(s);
    }

    std::string result;
    result.reserve(s.length()); // Pre-allocate with initial string length as a guess
    size_t currentPos = 0;
    size_t foundPos;

    while ((foundPos = s.find(target, currentPos)) != std::string_view::npos) {
        result.append(s.substr(currentPos, foundPos - currentPos));
        result.append(replacement);
        currentPos = foundPos + target.length();
    }
    result.append(s.substr(currentPos));
    return result;
}

std::string replaceFirst(std::string_view s, std::string_view from, std::string_view to) {
    auto pos = s.find(from);
    if (pos == std::string_view::npos) {
        return std::string(s);
    }
    std::string result;
    result.reserve(s.length() - from.length() + to.length()); // Pre-allocate
    result.append(s.substr(0, pos));
    result.append(to);
    result.append(s.substr(pos + from.length()));
    return result;
}

std::string replaceN(std::string_view s, std::string_view from, std::string_view to, size_t count) {
    if (from.empty() || count == 0) {
        return std::string(s);
    }

    std::string result;
    result.reserve(s.length()); // Pre-allocate
    size_t currentPos = 0;
    size_t replacements = 0;

    while (replacements < count) {
        auto foundPos = s.find(from, currentPos);
        if (foundPos == std::string_view::npos) {
            break;
        }
        result.append(s.substr(currentPos, foundPos - currentPos));
        result.append(to);
        currentPos = foundPos + from.length();
        replacements++;
    }

    result.append(s.substr(currentPos));
    return result;
}

std::string join(const std::vector<std::string>& parts, std::string_view delimiter) {
    if (parts.empty()) {
        return "";
    }

    // Calculate total length more accurately if possible, or just append and let it reallocate
    size_t totalLength = 0;
    for (const auto& part : parts) {
        totalLength += part.length();
    }
    if (parts.size() > 1) {
        totalLength += (parts.size() - 1) * delimiter.length();
    }
    std::string result;
    result.reserve(totalLength);

    auto it = parts.begin();
    result.append(*it);
    for (++it; it != parts.end(); ++it) {
        result.append(delimiter);
        result.append(*it);
    }
    return result;
}

std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty) {
    std::vector<std::string> tokens;
    if (s.empty()) {
        if (!skipEmpty) { // If s is empty and we don't skip empty, add an empty string.
            tokens.emplace_back("");
        }
        return tokens;
    }
    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string_view::npos) {
        std::string_view token = s.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.emplace_back(token);
        }
        start = end + 1;
        end = s.find(delimiter, start);
    }
    // Add the last token
    std::string_view token = s.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.emplace_back(token);
    }
    return tokens;
}

std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty) {
    std::vector<std::string> tokens;
    if (s.empty()) {
        if (!skipEmpty) { // If s is empty and we don't skip empty, add an empty string.
            tokens.emplace_back("");
        }
        return tokens;
    }
    if (delimiter.empty()) { // Splitting by empty delimiter returns the whole string as one token
        if (!skipEmpty) { // If delimiter is empty and we don't skip empty, add the whole string.
            tokens.emplace_back(s);
        }
        return tokens;
    }

    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string_view::npos) {
        std::string_view token = s.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.emplace_back(token);
        }
        start = end + delimiter.size();
        end = s.find(delimiter, start);
    }
    // Add the last token
    std::string_view token = s.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.emplace_back(token);
    }
    return tokens;
}

// Numeric Parsing/Validation

bool isInteger(std::string_view s) {
    if (s.empty()) return false;
    long long val; // Use long long to cover more range for checking
    const char* first = s.data();
    const char* last = s.data() + s.size();

    // Skip leading/trailing whitespace
    while (first < last && std::isspace(static_cast<unsigned char>(*first))) {
        ++first;
    }
    // Handle optional plus sign before parsing - std::from_chars should handle it, but explicit handling can prevent issues
    if (first < last && *first == '+') {
        ++first;
    }
    while (first < last && std::isspace(static_cast<unsigned char>(last[-1]))) {
        --last;
    }

    if (first == last) return false; // String was all whitespace or empty after trimming, or only a sign

    auto res = std::from_chars(first, last, val);
    return res.ptr == last && res.ec == std::errc();
}

bool isFloatingPoint(std::string_view s) {
    if (s.empty()) return false;
    double val;
    const char* first = s.data();
    const char* last = s.data() + s.size();

    // Skip leading/trailing whitespace
    while (first < last && std::isspace(static_cast<unsigned char>(*first))) {
        ++first;
    }
    // Handle optional plus sign before parsing
    if (first < last && *first == '+') {
        ++first;
    }
    while (first < last && std::isspace(static_cast<unsigned char>(last[-1]))) {
        --last;
    }

    if (first == last) return false; // String was all whitespace or empty after trimming, or only a sign

    auto res = std::from_chars(first, last, val);
    return res.ptr == last && res.ec == std::errc();
}

template <typename T>
Result<T> parseNumeric(std::string_view s, int base = default_radix) {
    if (s.empty()) {
        return std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    const char* first = s.data();
    const char* last = s.data() + s.size();

    // Skip leading/trailing whitespace
    while (first < last && std::isspace(static_cast<unsigned char>(*first))) {
        ++first;
    }
    // Handle optional plus sign before parsing
    if (first < last && *first == '+') {
        ++first;
    }
    while (first < last && std::isspace(static_cast<unsigned char>(last[-1]))) {
        --last;
    }

    if (first == last) { // String was all whitespace or empty after trimming, or only a sign
        return std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    T value;
    std::from_chars_result res;

    if constexpr (std::is_integral_v<T>) {
        res = std::from_chars(first, last, value, base);
    } else { // Floating point types
        res = std::from_chars(first, last, value);
        // Explicitly check for non-finite values (overflow to infinity or NaN)
        // that std::from_chars might not always explicitly flag as out_of_range.
        if (res.ec == std::errc() && !std::isfinite(value)) {
             res.ec = std::errc::result_out_of_range;
        }
    }

    if (res.ec == std::errc()) {
        if (res.ptr == last) {
            return value;
        }
        // Partial parse or junk at the end
        return std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    if (res.ec == std::errc::result_out_of_range) {
        return std::unexpected(make_error_code(UtilsError::outOfRange));
    }
    // std::errc::invalid_argument (no digits found, etc.)
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}


Result<long> toLong(std::string_view s, int base) {
    return parseNumeric<long>(s, base);
}

Result<double> toDouble(std::string_view s) {
    return parseNumeric<double>(s);
}

Result<bool> parseBool(std::string_view s) {
    std::string lowerS = toLower(s); // Use the locale-independent toLower
    if (lowerS == "true" || lowerS == "1") {
        return true;
    }
    if (lowerS == "false" || lowerS == "0") {
        return false;
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

Result<int> toInt(std::string_view s, int base) {
    return parseNumeric<int>(s, base);
}

Result<float> toFloat(std::string_view s) {
    return parseNumeric<float>(s);
}

} // namespace utils
