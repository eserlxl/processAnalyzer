// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_STRING_H
#define UTILS_STRING_H

#include "utils/Types.h"
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <format>

namespace utils {

// String Manipulation
Result<std::string> urlEncode(std::string_view s);
Result<std::string> urlDecode(std::string_view s);
Result<std::string> base64Encode(std::string_view s);
Result<std::string> base64Decode(std::string_view s);
Result<std::string> base64Encode(std::span<const std::byte> data);
Result<std::vector<std::byte>> base64DecodeToBytes(std::string_view s);
Result<std::string> generateUuid();
bool equalsIgnoreCase(std::string_view s1, std::string_view s2);
std::string trim(std::string_view s);
bool startsWith(std::string_view s, std::string_view prefix);
bool endsWith(std::string_view s, std::string_view suffix);
bool contains(std::string_view s, std::string_view substring);
bool startsWithIgnoreCase(std::string_view str, std::string_view prefix);
bool endsWithIgnoreCase(std::string_view str, std::string_view suffix);
bool containsIgnoreCase(std::string_view str, std::string_view subStr);
std::string toLower(std::string_view s);
std::string toUpper(std::string_view s);
std::string replace(std::string_view s, std::string_view target, std::string_view replacement);
std::string replaceFirst(std::string_view s, std::string_view from, std::string_view to);
std::string replaceN(std::string_view s, std::string_view from, std::string_view to, size_t count);
std::string join(const std::vector<std::string>& parts, std::string_view delimiter);

template<typename... Args>
std::string format(std::string_view fmt, Args&&... args) {
    return std::vformat(fmt, std::make_format_args(args...));
}

[[deprecated("Use std::format-based Utils::format instead.")]]
std::string formatStringDeprecated(const char* fmt, ...);

std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false);
std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false);

// Numeric Parsing/Validation
bool isInteger(std::string_view s);
bool isFloatingPoint(std::string_view s);
Result<long> toLong(std::string_view s, int base = 10);
[[deprecated("Use Result-based toLong instead.")]]
std::optional<long> toLongDeprecated(std::string_view s);
Result<double> toDouble(std::string_view s);
[[deprecated("Use Result-based toDouble instead.")]]
std::optional<double> toDoubleDeprecated(std::string_view s);
Result<bool> parseBool(std::string_view s);
[[deprecated("Use Result-based parseBool instead.")]]
std::optional<bool> parseBoolDeprecated(std::string_view s);
Result<int> toInt(std::string_view s, int base = 10);
[[deprecated("Use Result-based toInt instead.")]]
std::optional<int> toIntDeprecated(std::string_view s);
Result<float> toFloat(std::string_view s);
[[deprecated("Use Result-based toFloat instead.")]]
std::optional<float> toFloatDeprecated(std::string_view s);

} // namespace utils

#endif // UTILS_STRING_H
