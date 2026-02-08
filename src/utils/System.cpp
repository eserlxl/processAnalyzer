// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/System.h"
#include <cstdlib>
#include <cstdio>
#include <array>
#include <vector> // Implicitly needed for string concat sometimes
#include <sys/wait.h> // for WEXITSTATUS

namespace utils {

Result<std::string> getEnv(const std::string& name) {
    char* value = std::getenv(name.c_str());
    if (value) {
        return std::string(value);
    }
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

Result<CommandOutput> executeCommand(const std::string& command) {
    std::string commandRedirect = command + " 2>&1";
    FILE* pipe = popen(commandRedirect.c_str(), "r");
    if (!pipe) {
        return std::unexpected(make_error_code(UtilsError::commandExecutionError));
    }

    constexpr size_t kBufferSize = 128;
    std::array<char, kBufferSize> buffer;
    std::string stdoutStr;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        stdoutStr += buffer.data();
    }

    int pcloseResult = pclose(pipe);
    int exitCode = WEXITSTATUS(pcloseResult);

    return CommandOutput{.stdoutStr = stdoutStr, .stderrStr = "", .exitCode = exitCode};
}

bool executeCommandDeprecated(const std::string& command, std::string& stdoutStr, std::string& stderrStr, int& exitCode) {
    std::string commandRedirect = command + " 2>&1";
    FILE* pipe = popen(commandRedirect.c_str(), "r");
    if (!pipe) {
        stderrStr = "popen() failed!";
        return false;
    }

    constexpr size_t kBufferSize = 128;
    std::array<char, kBufferSize> buffer;
    stdoutStr.clear();
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        stdoutStr += buffer.data();
    }

    int pcloseResult = pclose(pipe);
    exitCode = WEXITSTATUS(pcloseResult);

    return true;
}

// Stubs
Result<void> setEnv(std::string_view name, std::string_view value) {
    (void)name;
    (void)value;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<void> unsetEnv(std::string_view name) {
    (void)name;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<std::filesystem::path> getCurrentWorkingDirectory() {
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path) {
    (void)path;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

} // namespace utils
