// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H

#include <system_error>
#include <expected>

namespace utils {

enum class UtilsError {
    none = 0,
    fileNotFound,
    permissionDenied,
    ioError,
    invalidArgument,
    // New error codes for better granularity
    unsupportedOperation, // e.g., attempting binary read on non-existent file
    pathError,            // for path manipulation failures
    commandExecutionError, // for specific command execution issues

    // More granular I/O errors
    fileAlreadyExists,
    directoryNotEmpty,
    notADirectory,
    notAFile,
    isADirectory, // trying to operate on dir as file
    diskFull,
    noSpaceOnDevice,

    // Path errors
    pathNotRelative,
    pathNotAbsolute,
    basePathNotAncestor, // For makeRelative
    invalidPathFormat,

    // String conversion errors
    invalidBase64Input,
    invalidUrlEncoding,
    invalidUuidFormat,

    // System interaction errors
    envVarNotFound,
    commandNotFound,
    commandFailed, // For executeCommand
    processSpawnFailure,
    permissionDeniedCwd, // For setCurrentWorkingDirectory

    // Time errors
    invalidTimeFormat,
    timeParseError,
};

std::error_code make_error_code(UtilsError e);

template <typename T>
using Result = std::expected<T, std::error_code>;

} // namespace utils

namespace std {
template <>
struct is_error_code_enum<utils::UtilsError> : true_type {};
}

#endif // UTILS_TYPES_H
