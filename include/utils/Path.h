// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_PATH_H
#define UTILS_PATH_H

#include "utils/Types.h"
#include <filesystem>
#include <string>
#include <vector>

namespace utils {

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

} // namespace utils

#endif // UTILS_PATH_H
