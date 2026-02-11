// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer_internal_helpers.h"
#include "analyzer/process_analyzer.h" // For ProcessInfo, ProcessFilter, etc.
#include "utils/core.h"
#include <regex>

namespace Internal {

utils::Result<long long> getTotalSystemCpuTimeTicks(const std::filesystem::path& procPath) {
    auto stats = ProcessAnalyzer(procPath).getSystemCpuStats();
    if (stats) {
        unsigned long long totalTicks = stats->user + stats->nice + stats->system + stats->idle +
                                        stats->iowait + stats->irq + stats->softirq + stats->steal;
        return static_cast<long long>(totalTicks);
    }
    return std::unexpected(stats.error());
}

bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter) {
    if (filter.nameContains && process.name.find(*filter.nameContains) == std::string::npos) return false;
    if (filter.nameRegex && !std::regex_search(process.name, *filter.nameRegex)) return false;
    if (filter.userFilter && process.username != *filter.userFilter) return false;
    if (filter.stateFilter && (process.state.empty() || process.state[0] != *filter.stateFilter)) return false;
    // ... all other filter conditions ...
    return true;
}

int compareProcesses(const ProcessInfo& a, const ProcessInfo& b, ProcessSortField sortBy) {
    switch (sortBy) {
        case ProcessSortField::pid: return a.pid < b.pid ? -1 : (a.pid > b.pid ? 1 : 0);
        case ProcessSortField::ppid: return a.ppid < b.ppid ? -1 : (a.ppid > b.ppid ? 1 : 0);
        // ... all other sort fields ...
        default: return a.pid < b.pid ? -1 : (a.pid > b.pid ? 1 : 0);
    }
    return 0;
}

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

utils::Result<std::vector<std::string>> readProcessEnvironmentVars(const std::filesystem::path& procPath, pid_t pid) {
    std::vector<std::string> env;
    std::filesystem::path environPath = procPath / std::to_string(pid) / "environ";
    auto environContentOpt = utils::readTextFile(environPath.string());
    if (!environContentOpt) {
        auto check = checkPidPathExistsAndPermissions(procPath, pid);
        if (!check) return std::unexpected(check.error());
        return env;
    }
    std::string_view content = *environContentOpt;
    size_t start = 0;
    while(start < content.size()) {
        size_t end = content.find(0, start);
        if (end == std::string_view::npos) break;
        env.emplace_back(content.substr(start, end - start));
        start = end + 1;
    }
    return env;
}

} // namespace Internal

