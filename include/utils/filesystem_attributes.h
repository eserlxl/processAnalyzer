// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#pragma once

#include "utils/types.h"
#include <filesystem>
#include <string>

namespace utils {

// Filesystem attributes
Result<std::filesystem::perms> getPermissions(const std::filesystem::path& path);
Result<void> setPermissions(const std::filesystem::path& path, std::filesystem::perms prms);
Result<void> addPermissions(const std::filesystem::path& path, std::filesystem::perms prms);
Result<void> removePermissions(const std::filesystem::path& path, std::filesystem::perms prms);

Result<bool> exists(const std::filesystem::path& path);
Result<bool> isFile(const std::filesystem::path& path);
Result<bool> isDirectory(const std::filesystem::path& path);
Result<bool> isSymlink(const std::filesystem::path& path);

Result<void> chown(const std::filesystem::path& path, const std::string& owner, const std::string& group);

Result<bool> isReadable(const std::filesystem::path& path);
Result<bool> isWritable(const std::filesystem::path& path);
Result<bool> isExecutable(const std::filesystem::path& path);

Result<uintmax_t> getFileSize(const std::filesystem::path& filePath);

} // namespace utils
