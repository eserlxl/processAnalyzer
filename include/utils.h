// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <system_error>
#include <expected>
#include <filesystem>
#include <optional>
#include <string_view>

namespace Utils {

enum class UtilsError {
    None = 0,
    FileNotFound,
    PermissionDenied,
    IOError,
    InvalidArgument,
    ParseError,
};

std::error_code make_error_code(UtilsError e);


template <typename T>
using Result = std::expected<T, std::error_code>;

// --- NEW APIs ---

// File I/O
Result<std::string> readTextFile(const std::filesystem::path& path);
Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content);
bool exists(const std::filesystem::path& path);
bool isFile(const std::filesystem::path& path);
bool isDirectory(const std::filesystem::path& path);
Result<void> appendToFile(const std::filesystem::path& path, std::string_view content);
Result<std::vector<std::string>> readLines(const std::filesystem::path& path);
Result<void> createDirectories(const std::filesystem::path& path);
Result<void> remove(const std::filesystem::path& path, bool recursive = false);
Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path);

// String Manipulation
std::string trim(std::string_view s);
bool startsWith(std::string_view s, std::string_view prefix);
bool endsWith(std::string_view s, std::string_view suffix);
bool contains(std::string_view s, std::string_view substring);
std::string toLower(std::string_view s);
std::string toUpper(std::string_view s);
std::string replace(std::string_view s, std::string_view target, std::string_view replacement);
std::string join(const std::vector<std::string>& parts, std::string_view delimiter);

std::vector<std::string> split(std::string_view s, char delimiter, bool skipEmpty = false);
std::vector<std::string> split(std::string_view s, std::string_view delimiter, bool skipEmpty = false);

// Numeric Parsing/Validation
bool isInteger(std::string_view s);
bool isFloatingPoint(std::string_view s);
std::optional<long> toLong(std::string_view s);
std::optional<double> toDouble(std::string_view s);

// System Interaction
std::optional<std::string> getEnv(const std::string& name);



} // namespace Utils

// Specialize std::is_error_code_enum
namespace std {
template <>
struct is_error_code_enum<Utils::UtilsError> : true_type {};
}

#endif // UTILS_H

