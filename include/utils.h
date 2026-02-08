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

    // More granular I/O errors
    fileAlreadyExists,
    directoryNotEmpty,
    notADirectory,
    notAFile,
    isADirectory, // trying to operate on dir as file
    diskFull,
    noSpaceOnDevice,

    // Path errors
    pathNotRelative,
    pathNotAbsolute,
    basePathNotAncestor, // For makeRelative
    invalidPathFormat,

    // String conversion errors
    invalidBase64Input,
    invalidUrlEncoding,
    invalidUuidFormat,

    // System interaction errors
    envVarNotFound,
    commandNotFound,
    commandFailed, // For executeCommand
    processSpawnFailure,
    permissionDeniedCwd, // For setCurrentWorkingDirectory

    // Time errors
    invalidTimeFormat,
    timeParseError,
};

std::error_code makeErrorCode(UtilsError e);


template <typename T>
using Result = std::expected<T, std::error_code>;

// --- NEW APIs ---

// File I/O
Result<std::string> readTextFile(const std::filesystem::path& path);
Result<std::vector<std::byte>> readBinaryFile(const std::filesystem::path& path);
Result<void> writeTextFile(const std::filesystem::path& path, std::string_view content);
Result<void> writeBinaryFile(const std::filesystem::path& path, std::span<const std::byte> content);
Result<void> writeTextFileAtomic(const std::filesystem::path& path, std::string_view content);
Result<void> writeBinaryFileAtomic(const std::filesystem::path& path, std::span<const std::byte> content);

Result<std::filesystem::perms> getPermissions(const std::filesystem::path& path);
Result<void> setPermissions(const std::filesystem::path& path, std::filesystem::perms prms);
Result<void> addPermissions(const std::filesystem::path& path, std::filesystem::perms prms);
Result<void> removePermissions(const std::filesystem::path& path, std::filesystem::perms prms);
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
Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group);
bool isReadable(const std::filesystem::path& path);
bool isWritable(const std::filesystem::path& path);
bool isExecutable(const std::filesystem::path& path);

// Enum to control directory traversal flow
enum class TraversalControl {
    Continue,    // Continue traversal normally
    skipDir,     // Skip current directory and its children, continue with siblings
    stop,        // Stop traversal immediately
};

// Callback type for directory traversal
using TraversalCallback = std::function<TraversalControl(const std::filesystem::directory_entry& entry)>;

// Options for directory traversal
struct TraversalOptions {
    bool recursive = true;          // Traverse subdirectories
    bool followSymlinks = false;    // Follow symbolic links to directories
    bool includeDirectories = true; // Include directories in the callback
    bool includeFiles = true;       // Include files in the callback
    int maxDepth = -1;              // Maximum recursion depth (-1 for unlimited)
};

// New directory traversal API
Result<void> traverseDirectory(const std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options = {});

Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination);
[[deprecated("Use Result-based moveFile instead.")]]
bool moveFileDeprecated(const std::filesystem::path& source, const std::filesystem::path& destination, std::error_code& ec);
Result<uintmax_t> getFileSize(const std::filesystem::path& filePath);
[[deprecated("Use Result-based getFileSize instead.")]]
std::optional<uintmax_t> getFileSizeDeprecated(const std::filesystem::path& filePath, std::error_code& ec);
[[deprecated("The old traverseDirectory is deprecated. Use the one that returns a Result and takes a TraversalControl callback.")]]
bool traverseDirectoryDeprecated(const std::filesystem::path& dirPath, const std::function<void(const std::filesystem::path&)>& callback, bool recursive = true);

// Path Manipulation
Result<std::filesystem::path> canonicalPath(const std::filesystem::path& path);
Result<std::filesystem::path> makeRelative(const std::filesystem::path& path, const std::filesystem::path& base);
bool pathsEquivalent(const std::filesystem::path& p1, const std::filesystem::path& p2);
std::filesystem::path getAbsolutePath(const std::filesystem::path& path);
std::string getFileName(const std::filesystem::path& path);
std::string getFileNameWithoutExtension(const std::filesystem::path& path);
std::string getFileExtension(const std::filesystem::path& path);
std::filesystem::path getParentPath(const std::filesystem::path& path);
std::filesystem::path joinPaths(const std::vector<std::filesystem::path>& paths);
Result<void> createSymlink(const std::filesystem::path& target, const std::filesystem::path& link);
Result<std::filesystem::path> readSymlink(const std::filesystem::path& link);
bool isSymlink(const std::filesystem::path& path);

// File Hashing
enum class HashAlgorithm { shA256, mD5, crC32, shA512 };
Result<std::string> calculateFileHash(const std::filesystem::path& path, HashAlgorithm algo);

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
Result<void> setEnv(std::string_view name, std::string_view value);
Result<void> unsetEnv(std::string_view name);
Result<std::filesystem::path> getCurrentWorkingDirectory();
Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path);

// Time Utilities
Result<std::chrono::system_clock::time_point> getCurrentSystemTime();
Result<std::chrono::steady_clock::time_point> getCurrentSteadyTime();
Result<std::string> formatTimestamp(std::chrono::system_clock::time_point tp, std::string_view formatStr);
Result<std::chrono::system_clock::time_point> parseTimestamp(std::string_view timestampStr, std::string_view formatStr);
std::string formatElapsedTime(long long seconds);
std::string formatTimestamp(long long unixTimestamp);

} // namespace Utils

// Specialize std::is_error_code_enum
namespace std {
template <>
struct is_error_code_enum<utils::UtilsError> : true_type {};
}

#endif // UTILS_H

