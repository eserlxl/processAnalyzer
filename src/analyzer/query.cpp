// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <algorithm>
#include <vector>
#include <string>
#include <regex>
#include <functional>

#include "analyzer/core.h"
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
        return {std::unexpected(pidsResult.error())};
    }

    std::vector<ProcessInfo> filteredProcesses;
    filteredProcesses.reserve(pidsResult.value().size()); // Reserve space for efficiency

    for (int pid : pidsResult.value()) {
        auto detailsResult = getProcessDetails(pid);
        if (!detailsResult) {
            // Log error or continue, depending on desired robustness.
            // For now, let's just skip processes we can't get details for.
            continue;
        }
        const ProcessInfo& pInfo = detailsResult.value(); // Use const reference

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

    // Sort processes.
    // compareLess implements the strict ordering for the selected field. For
    // descending order we swap the operands rather than negating the result:
    // negating would make comp(a,b) and comp(b,a) both true for equal keys,
    // violating the strict-weak-ordering precondition of std::ranges::sort.
    auto compareLess = [&](const ProcessInfo& a, const ProcessInfo& b) {
        switch (sortBy) {
            case ProcessSortField::pid:             return a.pid < b.pid;
            case ProcessSortField::ppid:            return a.ppid < b.ppid;
            case ProcessSortField::rss:             return a.residentMemory < b.residentMemory;
            case ProcessSortField::vmsize:          return a.virtualMemory < b.virtualMemory;
            case ProcessSortField::startTime:       return a.startTimeUnix < b.startTimeUnix;
            case ProcessSortField::cpuTime:         return (a.cpuUserTimeTicks + a.cpuKernelTimeTicks) < (b.cpuUserTimeTicks + b.cpuKernelTimeTicks);
            case ProcessSortField::name:            return a.name < b.name;
            case ProcessSortField::executablePath:  return a.executablePath < b.executablePath;
            case ProcessSortField::cmdline:         return a.cmdline < b.cmdline;
            case ProcessSortField::uid:             return a.uid < b.uid;
            case ProcessSortField::user:            return a.username < b.username;
            case ProcessSortField::state:           return a.state < b.state;
            case ProcessSortField::threads:         return a.threadCount < b.threadCount;
            case ProcessSortField::cwd:             return a.currentWorkingDirectory < b.currentWorkingDirectory;
            case ProcessSortField::cpuUserTime:     return a.cpuUserTimeTicks < b.cpuUserTimeTicks;
            case ProcessSortField::cpuKernelTime:   return a.cpuKernelTimeTicks < b.cpuKernelTimeTicks;
            case ProcessSortField::ioReadBytes:     return a.ioReadBytes < b.ioReadBytes;
            case ProcessSortField::ioWriteBytes:    return a.ioWriteBytes < b.ioWriteBytes;
            case ProcessSortField::priority:        return a.priority < b.priority;
            default:                                return a.pid < b.pid; // Default sort by PID
        }
    };
    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
            return (sortOrder == SortOrder::asc) ? compareLess(a, b) : compareLess(b, a);
        }
    );

    return filteredProcesses;
}