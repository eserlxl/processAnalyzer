// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/Types.h"
#include <string>
#include <system_error> // Required for std::errc

namespace utils {

class UtilsErrorCategory : public ::std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override {
        return "UtilsError";
    }

    [[nodiscard]] ::std::string message(int ev) const override {
        switch (static_cast<UtilsError>(ev)) {
            case UtilsError::none: return "Success";
            case UtilsError::fileNotFound: return "File not found";
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
            case UtilsError::noSpaceOnDevice: return "No space on device";
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
            default: return "Unknown error";
        }
    }

    [[nodiscard]] bool equivalent(int code, const ::std::error_condition& condition) const noexcept override {
        switch (static_cast<UtilsError>(code)) {
            case UtilsError::fileNotFound:
                return condition == ::std::errc::no_such_file_or_directory;
            case UtilsError::permissionDenied:
            case UtilsError::permissionDeniedCwd:
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
                return condition == ::std::errc::invalid_argument;
            case UtilsError::fileAlreadyExists:
                return condition == ::std::errc::file_exists;
            case UtilsError::directoryNotEmpty:
                return condition == ::std::errc::directory_not_empty;
            case UtilsError::notADirectory:
            case UtilsError::isADirectory:
                return condition == ::std::errc::is_a_directory; // Use is_a_directory for both: expecting file, got dir OR expecting dir, got file
            case UtilsError::diskFull:
            case UtilsError::noSpaceOnDevice:
                return condition == ::std::errc::no_space_on_device;
            case UtilsError::commandNotFound:
                return condition == ::std::errc::no_such_process; // Closest std::errc for command not found
            case UtilsError::commandExecutionError:
            case UtilsError::commandFailed:
            case UtilsError::processSpawnFailure:
                return condition == ::std::errc::operation_not_permitted; // Generic execution error
            case UtilsError::unsupportedOperation:
                return condition == ::std::errc::operation_not_supported;
            default:
                return false;
        }
    }
};

const UtilsErrorCategory& utilsCategory() {
    static UtilsErrorCategory instance;
    return instance;
}

// make_error_code and make_error_condition are automatically provided by std
// when specializing is_error_code_enum and is_error_condition_enum.
// No explicit definition needed here.

} // namespace utils

// No need to specialize std::is_error_code_enum or std::is_error_condition_enum here,
// as they are already done in the header.
// Also no need for an explicit make_error_code overload in the global namespace,
// as the specialization handles it.
