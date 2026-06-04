// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "analyzer/internal/helpers.h"
#include "utils/types.h"

#include <sys/resource.h>
#include <cerrno>

utils::Result<void> ProcessAnalyzer::setProcessPriority(int pid, int niceValue) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return std::unexpected(check.error());
    }

    if (::setpriority(PRIO_PROCESS, static_cast<id_t>(pid), niceValue) == 0) {
        return {};
    }

    switch (errno) {
        case ESRCH:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
        case EPERM:
        case EACCES:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        case EINVAL:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
        default:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
}
