// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "analyzer/internal/helpers.h"
#include "utils/core.h"
#include "utils/types.h"

#include <sys/resource.h>
#include <algorithm>
#include <cerrno>
#include <sstream>
#include <string>

namespace {

// Parse a CPU range-list token (e.g., "0-3" or "2") into individual core IDs.
void expandCpuToken(std::string_view token, std::vector<int>& out) {
    const auto dashPos = token.find('-');
    if (dashPos == std::string_view::npos) {
        if (auto v = utils::parseInteger<int>(token)) {
            out.push_back(*v);
        }
        return;
    }
    auto lo = utils::parseInteger<int>(token.substr(0, dashPos));
    auto hi = utils::parseInteger<int>(token.substr(dashPos + 1));
    if (lo && hi) {
        for (int cpu = *lo; cpu <= *hi; ++cpu) {
            out.push_back(cpu);
        }
    }
}

} // namespace

utils::Result<CpuSet> ProcessAnalyzer::getProcessCpuAffinity(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return std::unexpected(check.error());
    }

    const auto statusPath = procPath / std::to_string(pid) / "status";
    auto content = utils::readTextFile(statusPath.string());
    if (!content) {
        return std::unexpected(content.error());
    }

    std::istringstream iss{*content};
    std::string line;
    static constexpr std::string_view kPrefix = "Cpus_allowed_list:";
    while (std::getline(iss, line)) {
        if (!line.starts_with(kPrefix)) {
            continue;
        }
        const auto valueStr = utils::trim(line.substr(kPrefix.size()));
        CpuSet result;
        for (const auto& token : utils::split(valueStr, ',')) {
            expandCpuToken(utils::trim(token), result.cpus);
        }
        std::ranges::sort(result.cpus);
        return result;
    }

    return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

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
