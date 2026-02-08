// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_SYSTEM_H
#define UTILS_SYSTEM_H

#include "utils/Types.h"
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
Result<CommandOutput> executeCommand(const std::string& command);

[[deprecated("Use Result-based executeCommand instead.")]]
bool executeCommandDeprecated(const std::string& command, std::string& stdoutStr, std::string& stderrStr, int& exitCode);

Result<std::string> getEnv(const std::string& name);
Result<void> setEnv(std::string_view name, std::string_view value);
Result<void> unsetEnv(std::string_view name);
Result<std::filesystem::path> getCurrentWorkingDirectory();
Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path);

} // namespace utils

#endif // UTILS_SYSTEM_H
