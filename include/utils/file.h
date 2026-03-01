// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#pragma once

#include "utils/types.h"
#include <filesystem>
#include <string>
#include <vector>
#include <span>
#include <string_view>

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
Result<void> appendToFile(const std::filesystem::path& path, std::string_view content);

} // namespace utils
