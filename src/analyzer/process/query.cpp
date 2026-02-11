// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <algorithm>
#include <vector>
#include <string>
#include <regex>
#include <functional>

#include "analyzer/analyzer.h"
#include "analyzer/process_model.h"
#include "utils/types.h"

namespace {
// Helper to apply common string-based filters (contains, regex)
bool applyStringFilter(const std::optional<std::string>& containsFilter,
                       const std::optional<std::regex>& regexFilter,
                       const std::string& targetString) {
    if (containsFilter && targetString.find(*containsFilter) == std::string::npos) {
        return false;
    }
    if (regexFilter && !std::regex_search(targetString, *regexFilter)) {
        return false;
    }
    return true;
}

// Helper to apply numerical range filters
template<typename T>
bool applyRangeFilter(const std::optional<T>& minVal,
                      const std::optional<T>& maxVal,
                      const T& targetVal) {
    if (minVal && targetVal < *minVal) {
        return false;
    }
    if (maxVal && targetVal > *maxVal) {
        return false;
    }
    return true;
}
} // anonymous namespace

utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::queryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    auto pidsResult = getPids();
    if (!pidsResult) {
        return utils::Result<std::vector<ProcessInfo>>(std::unexpected(pidsResult.error()));
    }

    std::vector<ProcessInfo> filteredProcesses;
    for (int pid : pidsResult.value()) {
        auto detailsResult = getProcessDetails(pid);
        if (!detailsResult) {
            // Log error or continue, depending on desired robustness.
            // For now, let's just skip processes we can't get details for.
            continue;
        }
        ProcessInfo pInfo = detailsResult.value();

        // Apply filters
        if (!applyStringFilter(filter.nameContains, filter.nameRegex, pInfo.name)) continue;
        if (!applyStringFilter(filter.cmdlineContains, filter.cmdlineRegex, pInfo.cmdline)) continue;
        if (!applyStringFilter(filter.executablePathContains, filter.executablePathRegex, pInfo.executablePath)) continue;

        if (filter.userFilter && pInfo.username != *filter.userFilter) continue;
        if (filter.stateFilter && pInfo.state.front() != *filter.stateFilter) continue;
        if (filter.uidFilter && pInfo.uid != *filter.uidFilter) continue;

        if (!applyRangeFilter(filter.minThreads, filter.maxThreads, pInfo.threadCount)) continue;
        if (!applyRangeFilter(filter.minResidentMemoryKB, filter.maxResidentMemoryKB, pInfo.residentMemory)) continue;
        if (!applyRangeFilter(filter.minVirtualMemoryKB, filter.maxVirtualMemoryKB, pInfo.virtualMemory)) continue;
        if (!applyRangeFilter(filter.minPriority, filter.maxPriority, pInfo.priority)) continue;
        if (!applyRangeFilter(filter.minCpuUsage, filter.maxCpuUsage, pInfo.cpuUsage)) continue;
        if (!applyRangeFilter(filter.minMemoryPercentage, filter.maxMemoryPercentage, pInfo.memoryPercentage)) continue;

        if (filter.ppidFilter && pInfo.ppid != *filter.ppidFilter) continue;

        // Custom predicate filter
        if (filter.customPredicate && !(*filter.customPredicate)(pInfo)) {
            continue;
        }

        // Network connection filter
        if (filter.networkConnectionFilter) {
            auto connectionsResult = getNetworkConnections(pid);
            if (!connectionsResult) {
                // If we can't get network connections, assume it doesn't match the filter.
                continue;
            }
            bool networkMatch = false;
            for (const auto& conn : connectionsResult.value()) {
                const auto& netFilter = *filter.networkConnectionFilter;
                bool currentConnMatch = true;

                if (netFilter.localPort && conn.localPort != *netFilter.localPort) currentConnMatch = false;
                if (netFilter.remotePort && conn.remotePort != *netFilter.remotePort) currentConnMatch = false;
                if (netFilter.protocol && conn.protocol != *netFilter.protocol) currentConnMatch = false;
                if (netFilter.state && conn.state != *netFilter.state) currentConnMatch = false;

                if (netFilter.remoteAddressContains && conn.remoteAddress.find(*netFilter.remoteAddressContains) == std::string::npos) currentConnMatch = false;
                if (netFilter.remoteAddressRegex && !std::regex_search(conn.remoteAddress, *netFilter.remoteAddressRegex)) currentConnMatch = false;

                if (currentConnMatch) {
                    networkMatch = true;
                    break;
                }
            }
            if (!networkMatch) continue;
        }
        
        filteredProcesses.push_back(pInfo);
    }

    // Sort processes
    std::sort(filteredProcesses.begin(), filteredProcesses.end(),
        [&](const ProcessInfo& a, const ProcessInfo& b) {
            bool less = false;
            switch (sortBy) {
                case ProcessSortField::pid:             less = a.pid < b.pid; break;
                case ProcessSortField::ppid:            less = a.ppid < b.ppid; break;
                case ProcessSortField::rss:             less = a.residentMemory < b.residentMemory; break;
                case ProcessSortField::vmsize:          less = a.virtualMemory < b.virtualMemory; break;
                case ProcessSortField::startTime:       less = a.startTimeUnix < b.startTimeUnix; break;
                case ProcessSortField::cpuTime:         less = (a.cpuUserTimeTicks + a.cpuKernelTimeTicks) < (b.cpuUserTimeTicks + b.cpuKernelTimeTicks); break;
                case ProcessSortField::name:            less = a.name < b.name; break;
                case ProcessSortField::executablePath:  less = a.executablePath < b.executablePath; break;
                case ProcessSortField::cmdline:         less = a.cmdline < b.cmdline; break;
                case ProcessSortField::uid:             less = a.uid < b.uid; break;
                case ProcessSortField::user:            less = a.username < b.username; break;
                case ProcessSortField::state:           less = a.state < b.state; break;
                case ProcessSortField::threads:         less = a.threadCount < b.threadCount; break;
                case ProcessSortField::cwd:             less = a.currentWorkingDirectory < b.currentWorkingDirectory; break;
                case ProcessSortField::cpuUserTime:     less = a.cpuUserTimeTicks < b.cpuUserTimeTicks; break;
                case ProcessSortField::cpuKernelTime:   less = a.cpuKernelTimeTicks < b.cpuKernelTimeTicks; break;
                case ProcessSortField::ioReadBytes:     less = a.ioReadBytes < b.ioReadBytes; break;
                case ProcessSortField::ioWriteBytes:    less = a.ioWriteBytes < b.ioWriteBytes; break;
                case ProcessSortField::priority:        less = a.priority < b.priority; break;
                case ProcessSortField::cpuUsage:        less = a.cpuUsage < b.cpuUsage; break;
                case ProcessSortField::memoryPercentage:less = a.memoryPercentage < b.memoryPercentage; break;
                default: less = a.pid < b.pid; // Default sort by PID
            }
            return (sortOrder == SortOrder::asc) ? less : !less;
        }
    );

    return filteredProcesses;
}