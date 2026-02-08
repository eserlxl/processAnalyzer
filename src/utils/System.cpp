// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/System.h"

// C system headers
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h> // for WEXITSTATUS

// C++ standard library headers
#include <array>

namespace utils {

Result<std::string> getEnv(const std::string& name) {
    char* value = std::getenv(name.c_str());
    if (value) {
        return std::string(value);
    }
    // When std::getenv fails (variable not found), return `UtilsError::invalidArgument`.
    // This semantic choice treats a request for a non-existent variable as an invalid argument.
    return std::unexpected(make_error_code(UtilsError::invalidArgument));
}

Result<CommandOutput> executeCommand(const std::string& command) {
    // Redirect stderr to stdout so that all output is captured in stdoutStr.
    // As a consequence, CommandOutput::stderrStr will always be empty.
    std::string commandRedirect = "{ " + command + "; } 2>&1";
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



// Stubs
// setEnv: Not yet implemented. Returns `unsupportedOperation`.
Result<void> setEnv(std::string_view name, std::string_view value) {
    (void)name;
    (void)value;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

// unsetEnv: Not yet implemented. Returns `unsupportedOperation`.
Result<void> unsetEnv(std::string_view name) {
    (void)name;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

// getCurrentWorkingDirectory: Not yet implemented. Returns `unsupportedOperation`.
Result<std::filesystem::path> getCurrentWorkingDirectory() {
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

Result<void> setCurrentWorkingDirectory(const std::filesystem::path& path) {
    (void)path;
    return std::unexpected(make_error_code(UtilsError::unsupportedOperation));
}

} // namespace utils
