// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#pragma once

#include "utils/types.h"
#include <filesystem>
#include <string>
#include <vector>
#include <span>
#include <string_view>
#include <functional>

namespace utils {

// File I/O
/**
 * @brief Reads the entire content of a text file into a string.
 * @param path The path to the file.
 * @return A Result containing the file content as a string, or an error.
 * @note Assumes the file is encoded in UTF-8.
 */
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

// Changed return type from bool to Result<bool>
Result<bool> exists(const std::filesystem::path& path);
Result<bool> isFile(const std::filesystem::path& path);
Result<bool> isDirectory(const std::filesystem::path& path);

Result<void> appendToFile(const std::filesystem::path& path, std::string_view content);
Result<std::vector<std::string>> readLines(const std::filesystem::path& path);
Result<void> createDirectories(const std::filesystem::path& path);
/**
 * @brief Removes a file or directory.
 * @param path The path to remove.
 * @param recursive If true, removes a directory and its contents. If false, removal of a non-empty directory will fail.
 * @return A Result indicating success or failure.
 */
Result<void> remove(const std::filesystem::path& path, bool recursive = false);
Result<std::filesystem::path> createTemporaryFile(std::string_view prefix = "", std::string_view suffix = "");
Result<std::filesystem::path> createTemporaryDirectory(std::string_view prefix = "");
Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path);
Result<void> copyFile(const std::filesystem::path& source, const std::filesystem::path& destination);

/**
 * @brief Changes the owner and group of a file or directory.
 * @param path The path to the file or directory.
 * @param owner The new owner name. If empty, owner is not changed.
 * @param group The new group name. If empty, group is not changed.
 * @return A Result indicating success or failure.
 * @note This function is only available on POSIX-compliant systems.
 */
Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group);

/**
 * @brief Checks if a file is readable by the current user.
 * @param path The path to the file.
 * @return A Result containing true if readable, false otherwise, or an error if permissions cannot be checked.
 */
Result<bool> isReadable(const std::filesystem::path& path);

/**
 * @brief Checks if a file is writable by the current user.
 * @param path The path to the file.
 * @return A Result containing true if writable, false otherwise, or an error if permissions cannot be checked.
 */
Result<bool> isWritable(const std::filesystem::path& path);

/**
 * @brief Checks if a file is executable by the current user.
 * @param path The path to the file.
 * @return A Result containing true if executable, false otherwise, or an error if permissions cannot be checked.
 */
Result<bool> isExecutable(const std::filesystem::path& path);

// Enum to control directory traversal flow
enum class TraversalControl {
    Continue,    // Continue traversal normally.
    skipDir,     // When returned from a callback for a directory, the traversal will not recurse into that directory. It will continue with the next sibling.
    stop,        // Stop traversal immediately.
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

/**
 * @brief Traverses a directory and calls a callback for each entry.
 * @param dirPath The path to the directory to traverse.
 * @param callback The function to call for each entry.
 * @param options The options for the traversal.
 * @return A Result indicating success or failure.
 * @note The traversal is performed iteratively and is safe from stack overflow on deep directory structures.
 * Practical depth may still be limited by filesystem path length limits.
 */
Result<void> traverseDirectory(const std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options = {});

Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination);

Result<uintmax_t> getFileSize(const std::filesystem::path& filePath);

} // namespace utils
