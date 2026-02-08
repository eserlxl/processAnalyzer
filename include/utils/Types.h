// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H

#include <system_error>
#include <expected>
#include <string>

namespace utils {

// Define the Result type for consistent error handling
template <typename T>
using Result = ::std::expected<T, ::std::error_code>;

enum class UtilsError {
    none = 0,
    analyzerPermissionDenied,
    analyzerParsingError,
    analyzerProcessNotFound,
    analyzerSystemError,
    basePathNotAncestor,
    commandExecutionError,
    commandFailed,
    commandNotFound,
    directoryNotEmpty,
    diskFull,
    envVarNotFound,
    fileAlreadyExists,
    fileNotFound,
    fileTooLarge,
    invalidArgument,
    invalidBase64Input,
    invalidPathFormat,
    invalidTimeFormat,
    invalidUrlEncoding,
    invalidUuidFormat,
    ioError,
    isADirectory,
    noSpaceOnDevice,
    notAFile,
    notADirectory,
    outOfRange,
    pathError,
    pathNotAbsolute,
    pathNotRelative,
    permissionDenied,
    permissionDeniedCwd,
    processSpawnFailure,
    tempDirectoryError,
    timeParseError,
    traversalStopped,
    unsupportedOperation,
};

// Define the custom error category class and its methods directly in the header.
// This ensures UtilsErrorCategory is a complete type everywhere it's used.
class UtilsErrorCategory : public ::std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override {
        return "UtilsError";
    }

    [[nodiscard]] ::std::string message(int ev) const override {
        switch (static_cast<UtilsError>(ev)) {
            case UtilsError::none: return "No error";
            case UtilsError::fileNotFound: return "File or directory not found";
            case UtilsError::permissionDenied: return "Permission denied";
            case UtilsError::ioError: return "I/O error";
            case UtilsError::invalidArgument: return "Invalid argument";
            case UtilsError::unsupportedOperation: return "Unsupported operation";
            case UtilsError::pathError: return "Path error";
            case UtilsError::commandExecutionError: return "Command execution error";
            case UtilsError::fileAlreadyExists: return "File already exists";
            case UtilsError::directoryNotEmpty: return "Directory not empty";
            case UtilsError::notADirectory: return "Not a directory";
            case UtilsError::notAFile: return "Not a file";
            case UtilsError::isADirectory: return "Is a directory";
            case UtilsError::diskFull: return "Disk full";
            case UtilsError::noSpaceOnDevice: return "No space left on device";
            case UtilsError::fileTooLarge: return "File is too large to process";
            case UtilsError::pathNotRelative: return "Path is not relative";
            case UtilsError::pathNotAbsolute: return "Path is not absolute";
            case UtilsError::basePathNotAncestor: return "Base path is not an ancestor";
            case UtilsError::invalidPathFormat: return "Invalid path format";
            case UtilsError::invalidBase64Input: return "Invalid Base64 input";
            case UtilsError::invalidUrlEncoding: return "Invalid URL encoding";
            case UtilsError::invalidUuidFormat: return "Invalid UUID format";
            case UtilsError::envVarNotFound: return "Environment variable not found";
            case UtilsError::commandNotFound: return "Command not found";
            case UtilsError::commandFailed: return "Command failed";
            case UtilsError::processSpawnFailure: return "Process spawn failure";
            case UtilsError::permissionDeniedCwd: return "Permission denied for changing CWD";
            case UtilsError::invalidTimeFormat: return "Invalid time format";
            case UtilsError::timeParseError: return "Time parsing error";
            case UtilsError::traversalStopped: return "Directory traversal stopped by callback";
            case UtilsError::tempDirectoryError: return "Temporary directory error";
            case UtilsError::outOfRange: return "Value out of range";

            // Analyzer specific errors
            case UtilsError::analyzerProcessNotFound: return "Analyzer: Process not found";
            case UtilsError::analyzerParsingError: return "Analyzer: Parsing error";
            case UtilsError::analyzerSystemError: return "Analyzer: System error";
            case UtilsError::analyzerPermissionDenied: return "Analyzer: Permission denied";
            default: return "Unknown UtilsError";
        }
    }

    [[nodiscard]] bool equivalent(int code, const ::std::error_condition& condition) const noexcept override {
        switch (static_cast<UtilsError>(code)) {
            case UtilsError::none:
                return condition.value() == 0 && condition.category() == std::system_category();
            case UtilsError::fileNotFound:
                return condition == ::std::errc::no_such_file_or_directory;
            case UtilsError::permissionDenied:
            case UtilsError::permissionDeniedCwd:
            case UtilsError::analyzerPermissionDenied:
                return condition == ::std::errc::permission_denied;
            case UtilsError::ioError:
                return condition == ::std::errc::io_error;
            case UtilsError::invalidArgument:
            case UtilsError::pathError:
            case UtilsError::invalidPathFormat:
            case UtilsError::invalidBase64Input:
            case UtilsError::invalidUrlEncoding:
            case UtilsError::invalidUuidFormat:
            case UtilsError::invalidTimeFormat:
            case UtilsError::timeParseError:
            case UtilsError::pathNotRelative:
            case UtilsError::pathNotAbsolute:
            case UtilsError::basePathNotAncestor:
            case UtilsError::fileTooLarge:
            case UtilsError::analyzerParsingError:
                return condition == ::std::errc::invalid_argument;
            case UtilsError::outOfRange:
                return condition == ::std::errc::result_out_of_range;
            case UtilsError::fileAlreadyExists:
                return condition == ::std::errc::file_exists;
            case UtilsError::directoryNotEmpty:
                return condition == ::std::errc::directory_not_empty;
            case UtilsError::notADirectory:
            case UtilsError::isADirectory:
                return condition == ::std::errc::is_a_directory;
            case UtilsError::diskFull:
            case UtilsError::noSpaceOnDevice:
                return condition == ::std::errc::no_space_on_device;
            case UtilsError::commandNotFound:
            case UtilsError::analyzerProcessNotFound:
                return condition == ::std::errc::no_such_process;
            case UtilsError::commandExecutionError:
            case UtilsError::commandFailed:
            case UtilsError::processSpawnFailure:
            case UtilsError::analyzerSystemError:
                return condition == ::std::errc::operation_not_permitted;
            case UtilsError::unsupportedOperation:
            case UtilsError::traversalStopped:
            case UtilsError::tempDirectoryError:
                return condition == ::std::errc::operation_not_supported;
            default:
                return false;
        }
    }
};

// Define the accessor function for the custom error category inline in the header.
inline const UtilsErrorCategory& utilsErrorCategory() {
    static UtilsErrorCategory instance;
    return instance;
}

// Define make_error_code inline in the header.
inline ::std::error_code makeErrorCode(UtilsError e) {
    return {static_cast<int>(e), utilsErrorCategory()};
}

} // namespace utils

// Specializations must be in namespace std
namespace std {
template <>
struct is_error_code_enum<utils::UtilsError> : ::std::true_type {};
} // namespace std

#endif // UTILS_TYPES_H
