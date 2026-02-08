// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_FILE_H
#define UTILS_FILE_H

#include "utils/Types.h"
#include <filesystem>
#include <string>
#include <vector>
#include <span>
#include <string_view>
#include <functional>

namespace utils {

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

} // namespace utils

#endif // UTILS_FILE_H
