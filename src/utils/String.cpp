// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/String.h"
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <charconv>
#include <limits>
#include <optional>

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
                          return std::tolower(c1) == std::tolower(c2);
                      });
}

bool endsWithIgnoreCase(std::string_view str, std::string_view suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
                      [](unsigned char c1, unsigned char c2) {
                          return std::tolower(c1) == std::tolower(c2);
                      });
}

bool containsIgnoreCase(std::string_view str, std::string_view subStr) {
    auto it = std::ranges::search(str, subStr,
                          [](unsigned char ch1, unsigned char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
    return !it.empty();
}

std::string toLower(std::string_view s) {
    std::string result(s.length(), ' ');
    std::ranges::transform(s, result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string toUpper(std::string_view s) {
    std::string result(s.length(), ' ');
    std::ranges::transform(s, result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
    return result;
}

std::string replace(std::string_view s, std::string_view target, std::string_view replacement) {
    if (target.empty()) {
        return std::string(s);
    }

    std::string result;
    result.reserve(s.length());
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
    result.reserve(s.length() - from.length() + to.length());
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
    result.reserve(s.length());
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

    std::string result;
    size_t totalLength = 0;
    for (const auto& part : parts) {
        totalLength += part.length();
    }
    totalLength += (parts.size() - 1) * delimiter.length();
    result.reserve(totalLength);

    auto it = parts.begin();
    result.append(*it);
    for (++it; it != parts.end(); ++it) {
        result.append(delimiter);
        result.append(*it);
    }
    return result;
}

std::string formatStringDeprecated(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    va_list argsCopy;
    va_copy(argsCopy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, argsCopy);
    va_end(argsCopy);

    if (size < 0) {
        va_end(args);
        return "";
    }

    std::vector<char> buf(size + 1);
    std::vsnprintf(buf.data(), size + 1, fmt, args);
    
    va_end(args);
    return {buf.data(), static_cast<std::string::size_type>(size)};
}

std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty) {
    std::vector<std::string> tokens;
    if (s.empty()) {
        return tokens;
    }
    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string_view::npos) {
        auto token = s.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.emplace_back(token);
        }
        start = end + 1;
        end = s.find(delimiter, start);
    }
    auto token = s.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.emplace_back(token);
    }
    return tokens;
}

std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty) {
    std::vector<std::string> tokens;
    if (s.empty()) {
        return tokens;
    }
    if (delimiter.empty()) {
        if (!s.empty()) tokens.emplace_back(std::string(s));
        return tokens;
    }

    size_t start = 0;
    size_t end = s.find(delimiter);

    while (end != std::string_view::npos) {
        auto token = s.substr(start, end - start);
        if (!skipEmpty || !token.empty()) {
            tokens.emplace_back(token);
        }
        start = end + delimiter.size();
        end = s.find(delimiter, start);
    }
    auto token = s.substr(start);
    if (!skipEmpty || !token.empty()) {
        tokens.emplace_back(token);
    }
    return tokens;
}

// Stubs
Result<std::string> urlEncode(std::string_view s) {
    (void)s;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::string> urlDecode(std::string_view s) {
    (void)s;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::string> base64Encode(std::string_view s) {
    (void)s;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::string> base64Decode(std::string_view s) {
    (void)s;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::string> base64Encode(std::span<const std::byte> data) {
    (void)data;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::vector<std::byte>> base64DecodeToBytes(std::string_view s) {
    (void)s;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::string> generateUuid() {
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

bool equalsIgnoreCase(std::string_view s1, std::string_view s2) {
    (void)s1;
    (void)s2;
    return false;
}

// --- Numeric Parsing/Validation ---

bool isInteger(std::string_view s) {
    if (s.empty()) return false;
    size_t start = 0;
    if (s[0] == '-' || s[0] == '+') {
        start = 1;
    }
    if (start == s.size()) return false;
    return std::all_of(s.begin() + start, s.end(), [](unsigned char c){ return std::isdigit(c); });
}

bool isFloatingPoint(std::string_view s) {
    return toDouble(s).has_value();
}

Result<double> toDouble(std::string_view s) {
    double val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::unexpected(make_error_code(UtilsError::invalidArgument));

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

Result<long> toLong(std::string_view s, int base) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::unexpected(make_error_code(UtilsError::invalidArgument));

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val, base);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

std::optional<long> toLongDeprecated(std::string_view s) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::nullopt;

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::nullopt;
}

std::optional<double> toDoubleDeprecated(std::string_view s) {
    double val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::nullopt;

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        return val;
    }
    return std::nullopt;
}

Result<bool> parseBool(std::string_view s) {
    std::string lowerS = toLower(s);
    if (lowerS == "true" || lowerS == "1" || lowerS == "yes") {
        return true;
    }
    if (lowerS == "false" || lowerS == "0" || lowerS == "no") {
        return false;
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

std::optional<bool> parseBoolDeprecated(std::string_view s) {
    std::string lowerS = toLower(s);
    if (lowerS == "true" || lowerS == "1" || lowerS == "yes") {
        return true;
    }
    if (lowerS == "false" || lowerS == "0" || lowerS == "no") {
        return false;
    }
    return std::nullopt;
}

Result<int> toInt(std::string_view s, int base) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::unexpected(make_error_code(UtilsError::invalidArgument));

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val, base);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        if (val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max()) {
            return static_cast<int>(val);
        }
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

std::optional<int> toIntDeprecated(std::string_view s) {
    long val;
    std::string_view subS = s;
    if (!s.empty() && s[0] == '+') {
        subS = s.substr(1);
    }
    
    if (subS.empty()) return std::nullopt;

    auto res = std::from_chars(subS.data(), subS.data() + subS.size(), val);
    if (res.ec == std::errc() && res.ptr == subS.data() + subS.size()) {
        if (val >= std::numeric_limits<int>::min() && val <= std::numeric_limits<int>::max()) {
            return static_cast<int>(val);
        }
    }
    return std::nullopt;
}

Result<float> toFloat(std::string_view s) {
    double val;
    auto res = toDouble(s);
    if (res) {
        val = *res;
        if (val >= -std::numeric_limits<float>::max() && val <= std::numeric_limits<float>::max()) {
            return static_cast<float>(val);
        }
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

std::optional<float> toFloatDeprecated(std::string_view s) {
    double val;
    auto opt = toDouble(s);
    if (opt) {
        val = *opt;
        if (val >= -std::numeric_limits<float>::max() && val <= std::numeric_limits<float>::max()) {
            return static_cast<float>(val);
        }
    }
    return std::nullopt;
}

} // namespace utils
