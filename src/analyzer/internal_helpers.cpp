// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "internal_helpers.h"
#include "analyzer/analyzer_core.h" // For ProcessInfo, ProcessFilter, etc.
#include "utils/core.h"
#include <regex>

namespace Internal {

utils::Result<unsigned long long> getTotalSystemCpuTimeTicks() {
    auto stats = ProcessAnalyzer::getSystemCpuStats();
    if (stats) {
        unsigned long long totalTicks = stats->user + stats->nice + stats->system + stats->idle +
                                        stats->iowait + stats->irq + stats->softirq + stats->steal;
        return totalTicks;
    }
    return std::unexpected(stats.error());
}

bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter) {
    if (filter.nameContains && process.name.find(*filter.nameContains) == std::string::npos) return false;
    if (filter.nameRegex && !std::regex_search(process.name, *filter.nameRegex)) return false;
    if (filter.userFilter && process.username != *filter.userFilter) return false;
    if (filter.stateFilter && (process.state.empty() || process.state[0] != *filter.stateFilter)) return false;

    if (filter.minThreads && process.threadCount < *filter.minThreads) return false;
    if (filter.maxThreads && process.threadCount > *filter.maxThreads) return false;

    if (filter.minResidentMemoryKB && process.residentMemory < *filter.minResidentMemoryKB) return false;
    if (filter.maxResidentMemoryKB && process.residentMemory > *filter.maxResidentMemoryKB) return false;

    if (filter.minVirtualMemoryKB && process.virtualMemory < *filter.minVirtualMemoryKB) return false;
    if (filter.maxVirtualMemoryKB && process.virtualMemory > *filter.maxVirtualMemoryKB) return false;

    if (filter.cmdlineContains && process.cmdline.find(*filter.cmdlineContains) == std::string::npos) return false;
    if (filter.cmdlineRegex && !std::regex_search(process.cmdline, *filter.cmdlineRegex)) return false;

    if (filter.executablePathContains && process.executablePath.find(*filter.executablePathContains) == std::string::npos) return false;
    if (filter.executablePathRegex && !std::regex_search(process.executablePath, *filter.executablePathRegex)) return false;

    if (filter.uidFilter && process.uid != *filter.uidFilter) return false;

    if (filter.minPriority && process.priority < *filter.minPriority) return false;
    if (filter.maxPriority && process.priority > *filter.maxPriority) return false;

    if (filter.ppidFilter && process.ppid != *filter.ppidFilter) return false;

    if (filter.minCpuUsage && process.cpuUsage < *filter.minCpuUsage) return false;
    if (filter.maxCpuUsage && process.cpuUsage > *filter.maxCpuUsage) return false;

    if (filter.minMemoryPercentage && process.memoryPercentage < *filter.minMemoryPercentage) return false;
    if (filter.maxMemoryPercentage && process.memoryPercentage > *filter.maxMemoryPercentage) return false;

    if (filter.customPredicate && !(*filter.customPredicate)(process)) return false;

    return true;
}

enum class ComparisonResult : std::int8_t {
    less = -1,
    equal = 0,
    greater = 1
};

int compareProcesses(const ProcessInfo& a, const ProcessInfo& b, ProcessSortField sortBy) {
    switch (sortBy) {
        case ProcessSortField::pid:
            if (a.pid < b.pid) return static_cast<int>(ComparisonResult::less);
            if (a.pid > b.pid) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::ppid:
            if (a.ppid < b.ppid) return static_cast<int>(ComparisonResult::less);
            if (a.ppid > b.ppid) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::rss: // residentMemory
            if (a.residentMemory < b.residentMemory) return static_cast<int>(ComparisonResult::less);
            if (a.residentMemory > b.residentMemory) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::vmsize: // virtualMemory
            if (a.virtualMemory < b.virtualMemory) return static_cast<int>(ComparisonResult::less);
            if (a.virtualMemory > b.virtualMemory) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::startTime: // startTimeTicks
            if (a.startTimeTicks < b.startTimeTicks) return static_cast<int>(ComparisonResult::less);
            if (a.startTimeTicks > b.startTimeTicks) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::cpuTime: // cpuUserTimeTicks + cpuKernelTimeTicks
            {
                long long aCpuTime = a.cpuUserTimeTicks + a.cpuKernelTimeTicks;
                long long bCpuTime = b.cpuUserTimeTicks + b.cpuKernelTimeTicks;
                if (aCpuTime < bCpuTime) return static_cast<int>(ComparisonResult::less);
                if (aCpuTime > bCpuTime) return static_cast<int>(ComparisonResult::greater);
                return static_cast<int>(ComparisonResult::equal);
            }
        case ProcessSortField::name:
            return a.name.compare(b.name);
        case ProcessSortField::executablePath:
            return a.executablePath.compare(b.executablePath);
        case ProcessSortField::cmdline:
            return a.cmdline.compare(b.cmdline);
        case ProcessSortField::uid:
            if (a.uid < b.uid) return static_cast<int>(ComparisonResult::less);
            if (a.uid > b.uid) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::user: // username
            return a.username.compare(b.username);
        case ProcessSortField::state:
            return a.state.compare(b.state);
        case ProcessSortField::threads: // threadCount
            if (a.threadCount < b.threadCount) return static_cast<int>(ComparisonResult::less);
            if (a.threadCount > b.threadCount) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::cwd: // currentWorkingDirectory
            return a.currentWorkingDirectory.compare(b.currentWorkingDirectory);
        case ProcessSortField::cpuUserTime:
            if (a.cpuUserTimeTicks < b.cpuUserTimeTicks) return static_cast<int>(ComparisonResult::less);
            if (a.cpuUserTimeTicks > b.cpuUserTimeTicks) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::cpuKernelTime:
            if (a.cpuKernelTimeTicks < b.cpuKernelTimeTicks) return static_cast<int>(ComparisonResult::less);
            if (a.cpuKernelTimeTicks > b.cpuKernelTimeTicks) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::ioReadBytes:
            if (a.ioReadBytes < b.ioReadBytes) return static_cast<int>(ComparisonResult::less);
            if (a.ioReadBytes > b.ioReadBytes) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::ioWriteBytes:
            if (a.ioWriteBytes < b.ioWriteBytes) return static_cast<int>(ComparisonResult::less);
            if (a.ioWriteBytes > b.ioWriteBytes) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::priority:
            if (a.priority < b.priority) return static_cast<int>(ComparisonResult::less);
            if (a.priority > b.priority) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::cpuUsage:
            if (a.cpuUsage < b.cpuUsage) return static_cast<int>(ComparisonResult::less);
            if (a.cpuUsage > b.cpuUsage) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        case ProcessSortField::memoryPercentage:
            if (a.memoryPercentage < b.memoryPercentage) return static_cast<int>(ComparisonResult::less);
            if (a.memoryPercentage > b.memoryPercentage) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
        default:
            if (a.pid < b.pid) return static_cast<int>(ComparisonResult::less);
            if (a.pid > b.pid) return static_cast<int>(ComparisonResult::greater);
            return static_cast<int>(ComparisonResult::equal);
    }
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
        // If file content cannot be read, check if path exists and has permissions.
        // If it does, then the failure is in reading the content itself (e.g., empty or malformed).
        // Otherwise, propagate the error from checkPidPathExistsAndPermissions.
        auto check = checkPidPathExistsAndPermissions(procPath, pid);
        if (!check) return std::unexpected(check.error());
        // If path exists and is accessible, but content is empty, treat as parsing error for distinction.
        // An empty environ file means no environment variables, which is a valid scenario,
        // but utils::readTextFile returning nullopt for an *existing* file might imply
        // an issue with reading its content. Returning an empty vector is fine if the file
        // was truly empty, but the audit suggests distinguishing.
        // For now, if the file is truly empty, readTextFile *should* return an empty string_view, not nullopt.
        // So nullopt here implies a failure to read, hence analyzerParsingError.
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    std::string_view content = *environContentOpt;
    size_t start = 0;
    while(start < content.size()) {
        size_t end = content.find('\0', start);
        if (end == std::string_view::npos) break;
        env.emplace_back(content.substr(start, end - start));
        start = end + 1;
    }
    return env;
}

} // namespace Internal

