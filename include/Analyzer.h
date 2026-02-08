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
#include <chrono>       // For std::chrono
#include <map>          // For std::map

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

    // --- New Fields for Iteration 5 ---
    long long startTimeTicks; // Process start time in clock ticks since system boot
    std::string executablePath; // Path to the executable file (symlink /proc/<pid>/exe)
    std::string currentWorkingDirectory; // Current working directory (symlink /proc/<pid>/cwd)
    std::vector<std::string> environmentVariables; // Environment variables (from /proc/<pid>/environ)
    long long cpuUserTimeTicks; // User mode CPU time in clock ticks
    long long cpuKernelTimeTicks; // Kernel mode CPU time in clock ticks
    long long ioReadBytes;      // Total bytes read by the process (from /proc/<pid>/io)
    long long ioWriteBytes;     // Total bytes written by the process (from /proc/<pid>/io)
    int priority;               // Process priority (nice value)
};

struct ProcessCpuUsage {
    int pid;
    std::string name;
    double cpuPercentage; // CPU usage as a percentage (e.g., 50.5 for 50.5% CPU)
                          // Note: This is an instantaneous/delta percentage between two calls.
};

// New structures and enums for Iteration 3 filtering and sorting
struct ProcessFilter {
    std::optional<std::string> nameContains; // For name filter
    std::optional<std::string> userFilter;   // For user filter
    std::optional<char> stateFilter;         // 'R', 'S', 'Z', etc.
    
    // --- New Fields for Iteration 5 ---
    std::optional<int> minThreads;
    std::optional<int> maxThreads;
    std::optional<long long> minResidentMemoryKB; // Minimum Resident Set Size in KB
    std::optional<long long> maxResidentMemoryKB; // Maximum Resident Set Size in KB
    std::optional<long long> minVirtualMemoryKB;  // Minimum Virtual Memory Size in KB
    std::optional<long long> maxVirtualMemoryKB;  // Maximum Virtual Memory Size in KB
    std::optional<std::string> cmdlineContains;   // Filter by substring in command line
    std::optional<std::string> executablePathContains; // Filter by substring in executable path
    std::optional<uint32_t> uidFilter;             // Filter by exact UID
    std::optional<int> minPriority;                // Minimum nice value
    std::optional<int> maxPriority;                // Maximum nice value
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
    THREADS,
    // --- New Fields for Iteration 5 ---
    START_TIME,      // Sort by process start time
    EXECUTABLE_PATH, // Sort by executable path
    CWD,             // Sort by current working directory
    CPU_USER_TIME,   // Sort by user mode CPU time
    CPU_KERNEL_TIME, // Sort by kernel mode CPU time
    IO_READ_BYTES,   // Sort by total bytes read
    IO_WRITE_BYTES,  // Sort by total bytes written
    PRIORITY         // Sort by process priority (nice value)
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

    // New: Get the system's clock tick frequency (HZ)
    // Contract: Returns the system clock ticks per second.
    //           Typically 100 for older systems, 1000 for newer ones, but can vary.
    // Throws: std::runtime_error if sysconf(_SC_CLK_TCK) fails.
    static long getSystemClockTicksPerSecond();

    // New: Calculate CPU usage for a specific process over a given duration.
    // Contract: Returns the CPU usage percentage for the specified PID.
    //           This method will internally take two snapshots (before and after a delay)
    //           and calculate the delta.
    // Parameters:
    //   pid: The process ID to monitor.
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A ProcessCpuUsage object. `cpuPercentage` will be 0 if process not found or no change.
    // Throws: std::runtime_error if unable to get process details or if sleep fails.
    std::optional<ProcessCpuUsage> getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const;

    // New: Calculate CPU usage for all processes over a given duration.
    // Contract: Returns a vector of ProcessCpuUsage objects for all active processes.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A vector of ProcessCpuUsage objects. Processes that exit during the observation
    //          or have no CPU activity will have 0% usage.
    // Throws: std::runtime_error if unable to get process details or if sleep fails.
    std::vector<ProcessCpuUsage> getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const;
    
    // New: Get the parent process of a given PID.
    // Contract: Returns the ProcessInfo of the parent process.
    //           Returns std::nullopt if the parent process cannot be found or if PID is 0 (kernel).
    // Throws: std::runtime_error for permissions issues.
    std::optional<ProcessInfo> getParentProcess(int pid) const;

    // New: Get all descendant processes (children, grandchildren, etc.) of a given PID.
    // Contract: Returns a vector of ProcessInfo for all direct and indirect descendants.
    //           Returns empty vector if no descendants or PID not found.
    // Throws: std::runtime_error for permissions issues.
    std::vector<ProcessInfo> getAllDescendantProcesses(int pid) const;

    // New: Retrieve environment variables for a specific process.
    // Contract: Returns a vector of strings, where each string is an "KEY=VALUE" pair.
    //           Returns empty vector if no environment variables are found or inaccessible.
    // Throws: std::runtime_error for permissions issues.
    std::vector<std::string> getProcessEnvironment(int pid) const;

private:
    std::string procPath;
    // Store last known CPU times for delta calculation, accessible across calls if needed for efficiency
    mutable std::map<int, std::pair<long long, long long>> lastCpuTimes; // pid -> {user_ticks, kernel_ticks}
    // Need a way to also store total system CPU time to normalize
    mutable std::optional<long long> lastTotalSystemCpuTimeTicks; // Total system CPU time ticks
};

#endif
