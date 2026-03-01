// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#pragma once

#include "utils/types.h"
#include <filesystem>
#include <string>
#include <vector>
#include <functional>

namespace utils {

// Filesystem operations

Result<void> createDirectories(const std::filesystem::path& path);
Result<void> createSymlink(const std::filesystem::path& targetPath, const std::filesystem::path& linkPath);
Result<std::filesystem::path> readSymlink(const std::filesystem::path& linkPath);
Result<void> remove(const std::filesystem::path& path, bool recursive = false);
Result<std::filesystem::path> createTemporaryFile(std::string_view prefix = "", std::string_view suffix = "");
Result<std::filesystem::path> createTemporaryDirectory(std::string_view prefix = "");
Result<std::vector<std::filesystem::path>> listDirectory(const std::filesystem::path& path);
Result<void> copyFile(const std::filesystem::path& source, const std::filesystem::path& destination);



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

Result<void> traverseDirectory(const std::filesystem::path& dirPath, TraversalCallback callback, const TraversalOptions& options = {});

Result<void> moveFile(const std::filesystem::path& source, const std::filesystem::path& destination);



} // namespace utils
