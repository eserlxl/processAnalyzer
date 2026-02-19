// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/internal/helpers.h"
#include "utils/types.h"
#include <filesystem>
#include <string>

namespace Internal {

utils::Result<void> checkPidPathExistsAndPermissions(const std::filesystem::path& procPath, pid_t pid) {
    std::filesystem::path pidPath = procPath / std::to_string(pid);
    if (!std::filesystem::exists(pidPath)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
    }
    try {
        std::filesystem::directory_iterator testIter(pidPath);
    } catch (const std::filesystem::filesystem_error&) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return {};
}

} // namespace Internal
