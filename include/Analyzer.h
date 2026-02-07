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
    std::vector<ProcessInfo> findProcesses(ProcessPredicate predicate) const;
    std::vector<ProcessInfo> getProcessesByName(std::string_view name) const;
    std::vector<ProcessInfo> getProcessesByUser(std::string_view username) const;

    [[deprecated("Use snapshot() or findProcesses() instead")]]
    void printAllProcesses() const;
    
private:
    std::string procPath;
};

#endif
