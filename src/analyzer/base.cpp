// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/core.h"
#include "analyzer/internal/helpers.h" // For checkPidPathExistsAndPermissions
#include <filesystem>
#include <system_error>
#include <utility> // For std::move

namespace fs = std::filesystem;

ProcessAnalyzer::ProcessAnalyzer(std::filesystem::path procPath) : procPath(std::move(procPath)) {}

void ProcessAnalyzer::setProcPath(const std::filesystem::path& newPath) {
    procPath = newPath;
}

const std::filesystem::path& ProcessAnalyzer::getProcPath() const {
    return procPath;
}

utils::Result<std::vector<int>> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (utils::isInteger(filename)) {
                    if (auto parsedPid = utils::parseInteger<int>(filename)) {
                        pids.push_back(*parsedPid);
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Distinguish between permission denied and other filesystem errors
        if (e.code() == std::errc::permission_denied) {
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        }
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
    return pids;
}

std::generator<int> ProcessAnalyzer::streamPids() const {
    // Use the non-throwing std::error_code overloads so a filesystem error
    // (procPath missing, not a directory, or unreadable) ends the stream
    // gracefully instead of throwing std::filesystem_error out of the
    // coroutine -- matching getPids()'s guarded iteration.
    std::error_code ec;
    if (!fs::exists(procPath, ec) || ec) {
        co_return;
    }

    fs::directory_iterator it(procPath, ec);
    if (ec) {
        co_return;
    }
    const fs::directory_iterator end;
    while (it != end) {
        const auto& entry = *it;
        std::error_code entryEc;
        if (entry.is_directory(entryEc) && !entryEc) {
            std::string filename = entry.path().filename().string();
            if (utils::isInteger(filename)) {
                if (auto parsedPid = utils::parseInteger<int>(filename)) {
                    co_yield *parsedPid;
                }
            }
        }
        it.increment(ec);
        if (ec) {
            co_return;
        }
    }
}

utils::Result<ProcessInfo> ProcessAnalyzer::getParentProcess(int pid) const {
    auto processInfoResult = getProcessDetails(pid);
    if (!processInfoResult) {
        return std::unexpected(processInfoResult.error());
    }
    pid_t ppid = processInfoResult->ppid;
    if (ppid == 0 || ppid == 1) { // 0 for kernel threads, 1 for init/systemd
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound)); // No meaningful parent
    }
    return getProcessDetails(ppid);
}


utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::getChildProcesses(int pid) const {
    std::vector<ProcessInfo> children;
    
    // Check if the parent PID exists and we have permissions to access it
    auto checkParent = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!checkParent) {
        if (checkParent.error() == utils::make_error_code(utils::UtilsError::analyzerProcessNotFound)) {
            // If the parent process doesn't exist, it can't have children.
            // Return an empty list, not an error.
            return children;
        }
        return std::unexpected(checkParent.error());
    }

    auto pidsResult = getPids();
    if (!pidsResult) {
        return std::unexpected(pidsResult.error());
    }

    for (int currentPid : pidsResult.value()) {
        if (currentPid == pid) continue; // A process is not its own child
        
        auto processDetailsResult = getProcessDetails(currentPid);
        if (processDetailsResult) {
            if (processDetailsResult->ppid == pid) {
                children.push_back(processDetailsResult.value());
            }
        }
        // If we can't get details for a process, skip it, don't propagate error
    }
    return children;
}

// Implement getAllDescendantProcesses
utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::getAllDescendantProcesses(int pid) const {
    std::vector<ProcessInfo> descendants;
    std::vector<int> pidsToExplore;
    pidsToExplore.push_back(pid);

    std::vector<int> visitedPids;

    while (!pidsToExplore.empty()) {
        int currentParentPid = pidsToExplore.back();
        pidsToExplore.pop_back();

        // Avoid infinite loops with circular parent-child relationships (though rare in /proc)
        if (std::ranges::find(visitedPids, currentParentPid) != visitedPids.end()) {
            continue;
        }
        visitedPids.push_back(currentParentPid);

        auto childrenResult = getChildProcesses(currentParentPid);
        if (!childrenResult) {
            // Log error or continue, depending on desired robustness.
            // For now, if we can't get children for a process, skip it.
            continue;
        }

        for (const auto& child : childrenResult.value()) {
            descendants.push_back(child);
            pidsToExplore.push_back(child.pid); // Add children to explore their descendants
        }
    }
    return descendants;
}

// Implementation of snapshot
utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::snapshot() const {
    std::vector<ProcessInfo> processList;
    auto pidsResult = getPids();
    if (!pidsResult) {
        return std::unexpected(pidsResult.error());
    }

    for (int pid : pidsResult.value()) {
        auto detailsResult = getProcessDetails(pid);
        if (detailsResult) {
            processList.push_back(detailsResult.value());
        }
        // If we can't get details for a process, skip it.
    }
    return processList;
}

// Implement streamProcesses using streamPids and getProcessDetails
std::generator<ProcessInfo> ProcessAnalyzer::streamProcesses() const {
    for (int pid : streamPids()) {
        auto detailsResult = getProcessDetails(pid);
        if (detailsResult) {
            co_yield detailsResult.value();
        }
    }
}

// Implement streamQueryProcesses
std::generator<ProcessInfo> ProcessAnalyzer::streamQueryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    // This is essentially replicating the queryProcesses logic, but yielding.
    // For now, let's implement a simpler version without full sorting for streaming,
    // or call the existing queryProcesses and stream its result.
    // Given the task is just to get it to compile, let's just make it yield from queryProcesses
    // (which means it won't be truly streaming, but will work).
    // A proper streaming query would apply filters on the fly.

    // To make it truly streaming, we would replicate the filtering logic here
    // and yield matching processes. Sorting a stream is not trivial.
    // For compilation, let's just use the queryProcesses result.

    auto queryResult = queryProcesses(filter, sortBy, sortOrder);
    if (queryResult) {
        for (const auto& pInfo : queryResult.value()) {
            co_yield pInfo;
        }
    }
    // No co_yield if queryResult is an error.
}

