// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#ifndef UTILS_TYPES_H
#define UTILS_TYPES_H

#include <system_error>
#include <expected>
#include <string>
#include <type_traits> // Required for std::is_signed_v, std::make_unsigned_t

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
    unknownError,
};

// Define the custom error category class and its methods directly in the header.
// This ensures UtilsErrorCategory is a complete type everywhere it's used.
class UtilsErrorCategory : public ::std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override {
        return "UtilsError";
    }

    [[nodiscard]] ::std::string message(int ev) const override {
        switch (ev) { // Switch on integer value directly
            case static_cast<int>(UtilsError::none): return "No error";
            case static_cast<int>(UtilsError::fileNotFound): return "File or directory not found";
            case static_cast<int>(UtilsError::permissionDenied): return "Permission denied";
            case static_cast<int>(UtilsError::ioError): return "I/O error";
            case static_cast<int>(UtilsError::invalidArgument): return "Invalid argument";
            case static_cast<int>(UtilsError::unsupportedOperation): return "Unsupported operation";
            case static_cast<int>(UtilsError::pathError): return "Path error";
            case static_cast<int>(UtilsError::commandExecutionError): return "Command execution error";
            case static_cast<int>(UtilsError::fileAlreadyExists): return "File already exists";
            case static_cast<int>(UtilsError::directoryNotEmpty): return "Directory not empty";
            case static_cast<int>(UtilsError::notADirectory): return "Not a directory";
            case static_cast<int>(UtilsError::notAFile): return "Not a file";
            case static_cast<int>(UtilsError::isADirectory): return "Is a directory";
            case static_cast<int>(UtilsError::diskFull): return "Disk full";
            case static_cast<int>(UtilsError::noSpaceOnDevice): return "No space left on device";
            case static_cast<int>(UtilsError::fileTooLarge): return "File is too large to process";
            case static_cast<int>(UtilsError::pathNotRelative): return "Path is not relative";
            case static_cast<int>(UtilsError::pathNotAbsolute): return "Path is not absolute";
            case static_cast<int>(UtilsError::basePathNotAncestor): return "Base path is not an ancestor";
            case static_cast<int>(UtilsError::invalidPathFormat): return "Invalid path format";
            case static_cast<int>(UtilsError::invalidBase64Input): return "Invalid Base64 input";
            case static_cast<int>(UtilsError::invalidUrlEncoding): return "Invalid URL encoding";
            case static_cast<int>(UtilsError::invalidUuidFormat): return "Invalid UUID format";
            case static_cast<int>(UtilsError::envVarNotFound): return "Environment variable not found";
            case static_cast<int>(UtilsError::commandNotFound): return "Command not found";
            case static_cast<int>(UtilsError::commandFailed): return "Command failed";
            case static_cast<int>(UtilsError::processSpawnFailure): return "Process spawn failure";
            case static_cast<int>(UtilsError::permissionDeniedCwd): return "Permission denied for changing CWD";
            case static_cast<int>(UtilsError::invalidTimeFormat): return "Invalid time format";
            case static_cast<int>(UtilsError::timeParseError): return "Time parsing error";
            case static_cast<int>(UtilsError::traversalStopped): return "Directory traversal stopped by callback";
            case static_cast<int>(UtilsError::tempDirectoryError): return "Temporary directory error";
            case static_cast<int>(UtilsError::outOfRange): return "Value out of range";
            case static_cast<int>(UtilsError::unknownError): return "Unknown error";
            // Analyzer specific errors
            case static_cast<int>(UtilsError::analyzerProcessNotFound): return "Analyzer: Process not found";
            case static_cast<int>(UtilsError::analyzerParsingError): return "Analyzer: Parsing error";
            case static_cast<int>(UtilsError::analyzerSystemError): return "Analyzer: System error";
            case static_cast<int>(UtilsError::analyzerPermissionDenied): return "Analyzer: Permission denied";
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
            case UtilsError::pathError: // pathError grouped with invalid_argument
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
                return condition == ::std::errc::not_a_directory;
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
inline ::std::error_code make_error_code(UtilsError e) {
    return {static_cast<int>(e), utilsErrorCategory()};
}

} // namespace utils

// Specializations must be in namespace std
namespace std {
template <>
struct is_error_code_enum<utils::UtilsError> : ::std::true_type {};
} // namespace std

#endif // UTILS_TYPES_H
