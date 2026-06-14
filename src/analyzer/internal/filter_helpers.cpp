// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/internal/filter_helpers.h"
#include <regex>
#include <optional>
#include <string>

namespace {

bool applyStringFilter(const std::optional<std::string>& contains,
                       const std::optional<std::regex>& regexPat,
                       const std::string& target) {
    if (contains && !target.contains(*contains)) return false;
    if (regexPat && !std::regex_search(target, *regexPat)) return false;
    return true;
}

template<typename T>
bool applyRangeFilter(const std::optional<T>& minVal,
                      const std::optional<T>& maxVal,
                      const T& val) {
    if (minVal && val < *minVal) return false;
    if (maxVal && val > *maxVal) return false;
    return true;
}

} // anonymous namespace

namespace Internal {

bool passesStaticFilters(const ProcessFilter& filter, const ProcessInfo& pInfo) {
    if (!applyStringFilter(filter.nameContains, filter.nameRegex, pInfo.name)) return false;
    if (!applyStringFilter(filter.cmdlineContains, filter.cmdlineRegex, pInfo.cmdline)) return false;
    if (!applyStringFilter(filter.executablePathContains, filter.executablePathRegex, pInfo.executablePath)) return false;
    if (filter.userFilter && pInfo.username != *filter.userFilter) return false;
    if (filter.stateFilter && (pInfo.state.empty() || pInfo.state.front() != *filter.stateFilter)) return false;
    if (filter.uidFilter && pInfo.uid != *filter.uidFilter) return false;
    if (!applyRangeFilter(filter.minThreads, filter.maxThreads, pInfo.threadCount)) return false;
    if (!applyRangeFilter(filter.minResidentMemoryKB, filter.maxResidentMemoryKB, pInfo.residentMemory)) return false;
    if (!applyRangeFilter(filter.minVirtualMemoryKB, filter.maxVirtualMemoryKB, pInfo.virtualMemory)) return false;
    if (!applyRangeFilter(filter.minPriority, filter.maxPriority, pInfo.priority)) return false;
    if (filter.ppidFilter && pInfo.ppid != *filter.ppidFilter) return false;
    if (filter.customPredicate && !(*filter.customPredicate)(pInfo)) return false;
    return true;
}

bool passesNetworkFilter(const ProcessFilter::NetworkFilterCriteria& netFilter,
                         const std::vector<NetworkConnection>& conns) {
    for (const auto& conn : conns) {
        bool match = true;
        if (netFilter.localPort && conn.localPort != *netFilter.localPort) match = false;
        if (netFilter.remotePort && conn.remotePort != *netFilter.remotePort) match = false;
        if (netFilter.protocol && conn.protocol != *netFilter.protocol) match = false;
        if (netFilter.state && conn.state != *netFilter.state) match = false;
        if (netFilter.remoteAddressContains &&
            !conn.remoteAddress.contains(*netFilter.remoteAddressContains)) match = false;
        if (netFilter.remoteAddressRegex &&
            !std::regex_search(conn.remoteAddress, *netFilter.remoteAddressRegex)) match = false;
        if (match) return true;
    }
    return false;
}

} // namespace Internal
