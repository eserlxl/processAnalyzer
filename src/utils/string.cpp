// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/string.h"
#include <algorithm>

#include <charconv> // For std::from_chars
#include <limits>   // For std::numeric_limits
#include <cmath>    // For std::isinf, std::isfinite

namespace {

inline unsigned char toLowerAscii(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<unsigned char>(c + ('a' - 'A'));
    }
    return c;
}

inline unsigned char toUpperAscii(unsigned char c) {
    if (c >= 'a' && c <= 'z') {
        return static_cast<unsigned char>(c - ('a' - 'A'));
    }
    return c;
}

// std::from_chars accepts an integer base only in [minRadix, maxRadix]; any value
// outside this range is undefined behavior, so parseNumeric validates against it.
constexpr int minRadix = 2;
constexpr int maxRadix = 36;

} // namespace

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
    return s.contains(substring);
}

bool startsWithIgnoreCase(std::string_view str, std::string_view prefix) {
    if (prefix.length() > str.length()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), str.begin(),
                      [](unsigned char c1, unsigned char c2) {
                          return toLowerAscii(c1) == toLowerAscii(c2);
                      });
}

bool endsWithIgnoreCase(std::string_view str, std::string_view suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
                      [](unsigned char c1, unsigned char c2) {
                          return toLowerAscii(c1) == toLowerAscii(c2);
                      });
}

bool containsIgnoreCase(std::string_view str, std::string_view subStr) {
    if (subStr.empty()) {
        return true;
    }
    if (str.length() < subStr.length()) {
        return false;
    }

    auto it = std::ranges::search(
        str,
        subStr,
        [](unsigned char c1, unsigned char c2) {
            return toLowerAscii(c1) == toLowerAscii(c2);
        }
    );
    return !it.empty();
}

std::string toLower(std::string_view s) {
    std::string result(s.size(), '\0');
    std::ranges::transform(s, result.begin(), toLowerAscii);
    return result;
}

std::string toUpper(std::string_view s) {
    std::string result(s.size(), '\0');
    std::ranges::transform(s, result.begin(), toUpperAscii);
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
    if (from.empty()) {
        return std::string(s);
    }
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
    if (delimiter.empty()) { // Splitting by empty delimiter now splits into characters.
        tokens.reserve(s.size());
        for(char c : s) {
            tokens.emplace_back(1, c);
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

template <typename T>
Result<T> parseNumeric(std::string_view s, int base) {
    constexpr std::string_view whitespace = " \t\n\r\f\v";
    auto firstCharPos = s.find_first_not_of(whitespace);
    if (firstCharPos == std::string_view::npos) {
        return std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    auto lastCharPos = s.find_last_not_of(whitespace);
    std::string_view trimmedSv = s.substr(firstCharPos, lastCharPos - firstCharPos + 1);

    if (!trimmedSv.empty() && trimmedSv.front() == '+') {
        trimmedSv.remove_prefix(1);
    }

    T value{}; // Value-initialize
    std::from_chars_result res;
    const char* first = trimmedSv.data();
    const char* last = trimmedSv.data() + trimmedSv.size();

    if constexpr (std::is_integral_v<T>) {
        // std::from_chars has undefined behavior for any base outside [minRadix,
        // maxRadix]; the public toInt/toLong accept an arbitrary int base, so
        // reject an out-of-range value here rather than forwarding it (mirrors the
        // guard in the header-only parseInteger).
        if (base < minRadix || base > maxRadix) {
            return std::unexpected(make_error_code(UtilsError::invalidArgument));
        }
        res = std::from_chars(first, last, value, base);
    } else { // Floating point types
        res = std::from_chars(first, last, value);
    }

    if (res.ec == std::errc()) {
        // For floating point, std::from_chars may successfully parse non-finite
        // values like "inf" or "nan". We explicitly reject them.
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(value)) {
                return std::unexpected(make_error_code(UtilsError::invalidArgument));
            }
        }
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

bool isInteger(std::string_view s) {
    return parseNumeric<long long>(s, utils::defaultRadix).has_value();
}

bool isFloatingPoint(std::string_view s) {
    return parseNumeric<double>(s, utils::defaultRadix).has_value();
}


Result<long> toLong(std::string_view s, int base) {
    return parseNumeric<long>(s, base);
}

Result<double> toDouble(std::string_view s) {
    return parseNumeric<double>(s, utils::defaultRadix);
}

Result<bool> parseBool(std::string_view s) {
    auto equalsIgnoreCase = [](std::string_view s1, std::string_view s2) {
        if (s1.length() != s2.length()) return false;
        return std::equal(s1.begin(), s1.end(), s2.begin(), [](unsigned char c1, unsigned char c2) {
             return toLowerAscii(c1) == toLowerAscii(c2);
        });
    };

    // Trim leading and trailing whitespace for consistency with numeric parsing
    auto first = s.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string_view::npos) {
        return std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    auto last = s.find_last_not_of(" \t\n\r\f\v");
    std::string_view trimmed = s.substr(first, (last - first + 1));

    if (equalsIgnoreCase(trimmed, "true") || trimmed == "1") {
        return true;
    }
    if (equalsIgnoreCase(trimmed, "false") || trimmed == "0") {
        return false;
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

Result<int> toInt(std::string_view s, int base) {
    return parseNumeric<int>(s, base);
}

Result<float> toFloat(std::string_view s) {
    return parseNumeric<float>(s, utils::defaultRadix);
}

} // namespace utils
