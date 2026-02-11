// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/system.h"

// C system headers
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/wait.h> // for WEXITSTATUS
#include <unistd.h>   // for unlink, close

// C++ standard library headers
#include <array>
#include <string>
#include <vector>

namespace utils {

namespace {
    constexpr int bufferSize = 4096;
    constexpr int signalExitCodeBase = 128;
} // namespace

Result<::std::string> getEnv(::std::string_view name) {
    if (name.empty() || name.find('=') != ::std::string_view::npos ||
        name.find('\0') != ::std::string_view::npos) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // std::getenv requires a null-terminated string, so we might need to convert.
    // A string_view doesn't guarantee null termination.
    char* value = ::std::getenv(::std::string(name).c_str());
    if (value) {
        return ::std::string(value);
    }
    return ::std::unexpected(make_error_code(UtilsError::envVarNotFound));
}

Result<CommandOutput> executeCommand(::std::string_view command) {
    if (command.empty() || command.find('\0') != ::std::string_view::npos) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    // Create a temporary file for stderr using a safer method
    std::string stderrPathStr = "/tmp/process-analyzer-stderr-XXXXXX";
    int stderrFd = mkstemp(stderrPathStr.data());
    if (stderrFd == -1) {
        return ::std::unexpected(make_error_code(UtilsError::commandExecutionError));
    }
    close(stderrFd); // We only needed the unique name, not to keep it open.

    // Construct the command to redirect stderr. We use grouping to ensure
    // that stderr from the entire command is captured, even with pipes.
    ::std::string fullCommand = "{ " + ::std::string(command) + "; } 2> " + stderrPathStr;

    FILE* pipe = popen(fullCommand.c_str(), "r");
    if (!pipe) {
        unlink(stderrPathStr.c_str());
        return ::std::unexpected(make_error_code(UtilsError::commandExecutionError));
    }

    ::std::string stdoutStr;
    ::std::array<char, bufferSize> buffer;
    while (size_t bytesRead = fread(buffer.data(), 1, buffer.size(), pipe)) {
        stdoutStr.append(buffer.data(), bytesRead);
    }

    int pcloseResult = pclose(pipe);
    int exitCode = -1;
    if (pcloseResult == -1) {
        // pclose failed.
        exitCode = -1; // Indicate a pclose error.
    } else {
        if (WIFEXITED(pcloseResult)) {
            exitCode = WEXITSTATUS(pcloseResult);
        } else if (WIFSIGNALED(pcloseResult)) {
            // If terminated by a signal, return 128 + signal number, a common convention.
            exitCode = signalExitCodeBase + WTERMSIG(pcloseResult);
        }
    }

    // Read stderr from the temporary file
    ::std::string stderrStr;
    FILE* stderrFile = fopen(stderrPathStr.c_str(), "r");
    if (stderrFile) {
        while (size_t bytesRead = fread(buffer.data(), 1, buffer.size(), stderrFile)) {
            stderrStr.append(buffer.data(), bytesRead);
        }
        fclose(stderrFile);
    }

    // Clean up the temporary file
    unlink(stderrPathStr.c_str());

    return CommandOutput{.stdoutStr = stdoutStr, .stderrStr = stderrStr, .exitCode = exitCode};
}



// Stubs
// setEnv: Sets or modifies an environment variable.
// Not yet implemented. Returns `unsupportedOperation`.
Result<void> setEnv(::std::string_view name, ::std::string_view value) {
    if (name.empty() || name.find('=') != ::std::string_view::npos ||
        name.find('\0') != ::std::string_view::npos ||
        value.find('\0') != ::std::string_view::npos) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    const ::std::string nameStr(name);
    const ::std::string valueStr(value);
    if (::setenv(nameStr.c_str(), valueStr.c_str(), 1) != 0) {
        return ::std::unexpected(::std::error_code(errno, ::std::generic_category()));
    }
    return {};
}

// unsetEnv: Unsets or removes an environment variable.
// Not yet implemented. Returns `unsupportedOperation`.
Result<void> unsetEnv(::std::string_view name) {
    if (name.empty() || name.find('=') != ::std::string_view::npos ||
        name.find('\0') != ::std::string_view::npos) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }

    const ::std::string nameStr(name);
    if (::unsetenv(nameStr.c_str()) != 0) {
        return ::std::unexpected(::std::error_code(errno, ::std::generic_category()));
    }
    return {};
}

// getCurrentWorkingDirectory: Retrieves the application's current working directory.
// Not yet implemented. Returns `unsupportedOperation`.
Result<::std::filesystem::path> getCurrentWorkingDirectory() {
    ::std::error_code ec;
    auto cwd = ::std::filesystem::current_path(ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return cwd;
}

// setCurrentWorkingDirectory: Changes the application's current working directory.
// Not yet implemented. Returns `unsupportedOperation`.
Result<void> setCurrentWorkingDirectory(const ::std::filesystem::path& path) {
    if (path.empty()) {
        return ::std::unexpected(make_error_code(UtilsError::invalidArgument));
    }
    ::std::error_code ec;
    ::std::filesystem::current_path(path, ec);
    if (ec) {
        return ::std::unexpected(ec);
    }
    return {};
}

} // namespace utils
