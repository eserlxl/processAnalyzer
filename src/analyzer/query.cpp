// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <algorithm>
#include <utility>
#include <vector>
#include <string>

#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "analyzer/internal/filter_helpers.h"
#include "utils/types.h"

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
    filteredProcesses.reserve(pidsResult.value().size());

    for (int pid : pidsResult.value()) {
        auto detailsResult = getProcessDetails(pid);
        if (!detailsResult) continue;
        const ProcessInfo& pInfo = detailsResult.value();

        if (!Internal::passesStaticFilters(filter, pInfo)) continue;

        if (filter.networkConnectionFilter) {
            auto connectionsResult = getNetworkConnections(pid);
            if (!connectionsResult ||
                !Internal::passesNetworkFilter(*filter.networkConnectionFilter, *connectionsResult)) {
                continue;
            }
        }

        filteredProcesses.push_back(pInfo);
    }

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
        }
        std::unreachable();
    };
    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
            return (sortOrder == SortOrder::asc) ? compareLess(a, b) : compareLess(b, a);
        }
    );

    return filteredProcesses;
}
