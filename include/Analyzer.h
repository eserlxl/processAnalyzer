// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <string_view>
#include <vector>
#include <optional>     // For std::optional
#include <functional>   // For std::function
#include <cstdint>      // For uint32_t

struct ProcessInfo {
    int pid;
    int ppid;
    uint32_t uid;
    std::string username;
    std::string name;
    std::string state;
    long residentMemory; // in KB (formerly memory_usage)
    long virtualMemory;  // in KB
    int threadCount;
    std::string cmdline;
};

// New structures and enums for Iteration 3 filtering and sorting
struct ProcessFilter {
    std::optional<std::string> nameContains; // For name filter
    std::optional<std::string> userFilter;   // For user filter
    std::optional<char> stateFilter;         // 'R', 'S', 'Z', etc.
};

enum class ProcessSortField {
    PID,
    PPID, // Though not directly sortable from command line yet, useful internally
    UID,
    USER,
    NAME,
    STATE,
    RSS,
    VM,
    THREADS
};

enum class SortOrder {
    ASC,
    DESC
};

// Predicate for filtering processes
using ProcessPredicate = std::function<bool(const ProcessInfo&)>;

class ProcessAnalyzer {
public:
    explicit ProcessAnalyzer(std::string_view procPath = "/proc");
    
    // Core API
    std::vector<int> getPids() const;
    std::optional<ProcessInfo> getProcessDetails(int pid) const;
    std::vector<ProcessInfo> snapshot() const;

    // Filtering API
    std::vector<ProcessInfo> findProcesses(const ProcessPredicate& predicate) const;
    std::vector<ProcessInfo> getProcessesByName(std::string_view name) const;
    std::vector<ProcessInfo> getProcessesByUser(std::string_view username) const;

    // New method for general process query with filtering and sorting
    // Contract: Returns processes matching filter, sorted as specified.
    // Throws: std::runtime_error if procPath is invalid or permissions issues.
    std::vector<ProcessInfo> queryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::PID,
        SortOrder sortOrder = SortOrder::ASC
    ) const;

    // New method to get child processes
    // Contract: Returns a vector of ProcessInfo for direct children of the given PID.
    //           Returns empty vector if no children or PID not found.
    // Throws: std::runtime_error for permissions issues.
    std::vector<ProcessInfo> getChildProcesses(int pid) const;

    // New method to get open files for a process
    // Contract: Returns a vector of strings, each representing an open file path.
    //           Returns empty vector if no files or PID not found/inaccessible.
    // Throws: std::runtime_error for permissions issues.
    std::vector<std::string> getProcessOpenFiles(int pid) const;


private:
    std::string procPath;
};

#endif
