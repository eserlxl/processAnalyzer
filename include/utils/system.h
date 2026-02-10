// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_SYSTEM_H
#define UTILS_SYSTEM_H

#include "utils/types.h"
#include <string>
#include <string_view>
#include <filesystem>

namespace utils {

// System Interaction
struct CommandOutput {
    std::string stdoutStr;
    std::string stderrStr;
    int exitCode;
};
Result<CommandOutput> executeCommand(std::string_view command);



Result<std::string> getEnv(std::string_view name);
Result<void> setEnv(std::string_view name, std::string_view value);
Result<void> unsetEnv(std::string_view name);
Result<std::filesystem::path> getCurrentWorkingDirectory();
Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path);

} // namespace utils

#endif // UTILS_SYSTEM_H
