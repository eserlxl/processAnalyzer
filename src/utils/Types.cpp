// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/Types.h"
#include <string>

namespace utils {

class UtilsErrorCategory : public std::error_category {
public:
    [[nodiscard]] const char* name() const noexcept override {
        return "UtilsError";
    }

    [[nodiscard]] std::string message(int ev) const override {
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
};

const UtilsErrorCategory& utilsCategory() {
    static UtilsErrorCategory instance;
    return instance;
}

std::error_code make_error_code(UtilsError e) {
    return {static_cast<int>(e), utilsCategory()};
}

} // namespace utils

// Definition for std::make_error_code overload if needed in global scope,
// but usually it's found via ADL if in the same namespace as UtilsError.
// However, UtilsError is in utils namespace, so make_error_code should be there too (which it is).
// We also need to ensure the std specialization works.
namespace std {
    // Already specialized in header.
}
