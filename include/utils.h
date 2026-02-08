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
#include <functional>
#include <format>

namespace utils {

enum class UtilsError {
    none = 0,
    fileNotFound,
    permissionDenied,
    ioError,
    invalidArgument,
    // New error codes for better granularity
    unsupportedOperation, // e.g., attempting binary read on non-existent file
    pathError,            // for path manipulation failures
    commandExecutionError, // for specific command execution issues
};

std::error_code make_error_code(UtilsError e);


template <typename T>
using Result = std::expected<T, std::error_code>;

// --- NEW APIs ---

// File I/O
Result<std::string> readTextFile(const std::filesystem::path& path);
Result<std::vector<std::byte>> readBinaryFile(const std::filesystem::path& path);
Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content);
Result<void> writeBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content);
Result<void> appendToBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content);
bool exists(const std::filesystem::path& path);
bool isFile(const std::filesystem::path& path);
bool isDirectory(const std::filesystem::path& path);
Result<void> appendToFile(const std::filesystem::path& path, std::string_view content);
Result<std::vector<std::string>> readLines(const std::filesystem::path& path);
Result<void> createDirectories(const std::filesystem::path& path);
Result<void> remove(const std::filesystem::path& path, bool recursive = false);
Result<std::filesystem::path> createTemporaryFile(std::string_view prefix = "", std::string_view suffix = "");
Result<std::filesystem::path> createTemporaryDirectory(std::string_view prefix = "");
Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path);
Result<void> copyFile(const std::filesystem::path& source, const std::filesystem::path& destination);
[[deprecated("Use Result-based copyFile instead.")]]
bool copyFileDeprecated(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& ec);
Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination);
[[deprecated("Use Result-based moveFile instead.")]]
bool moveFileDeprecated(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& ec);
Result<uintmax_t> getFileSize(const std::filesystem::path& filePath);
[[deprecated("Use Result-based getFileSize instead.")]]
std::optional<uintmax_t> getFileSizeDeprecated(const std::filesystem::path& filePath, std::error_code& ec);
[[deprecated("The old traverseDirectory is deprecated. Use the one that returns a Result and takes a TraversalControl callback.")]]
bool traverseDirectoryDeprecated(const std::filesystem::path& dirPath, const std::function<void(const std::filesystem::path&)>& callback, bool recursive = true);

// Path Manipulation
std::filesystem::path getAbsolutePath(const std::filesystem::path& path);
std::string getFileName(const std::filesystem::path& path);
std::string getFileNameWithoutExtension(const std::filesystem::path& path);
std::string getFileExtension(const std::filesystem::path& path);
std::filesystem::path getParentPath(const std::filesystem::path& path);
std::filesystem::path joinPaths(const std::vector<std::filesystem::path>& paths);

// File Hashing
enum class HashAlgorithm { shA256, mD5, crC32 };
Result<std::string> calculateFileHash(const std::filesystem::path& path, HashAlgorithm algo);

// String Manipulation
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

// System Interaction
struct CommandOutput {
    std::string stdoutStr;
    std::string stderrStr;
    int exitCode;
};
Result<CommandOutput> executeCommand(const std::string& command);
[[deprecated("Use Result-based executeCommand instead.")]]
bool executeCommandDeprecated(const std::string& command, std::string& stdoutStr, std::string& stderrStr, int& exitCode);

Result<std::string> getEnv(const std::string& name);

// Time Utilities
std::string formatElapsedTime(long long seconds);
std::string formatTimestamp(long long unixTimestamp);

} // namespace Utils

// Specialize std::is_error_code_enum
namespace std {
template <>
struct is_error_code_enum<utils::UtilsError> : true_type {};
}

#endif // UTILS_H

