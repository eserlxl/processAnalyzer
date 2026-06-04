// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "analyzer/internal/helpers.h"
#include "utils/types.h"

#include <csignal>
#include <cerrno>

utils::Result<void> ProcessAnalyzer::sendSignal(int pid, int signal) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return std::unexpected(check.error());
    }

    if (::kill(static_cast<pid_t>(pid), signal) == 0) {
        return {};
    }

    switch (errno) {
        case ESRCH:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
        case EPERM:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        case EINVAL:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
        default:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
}
