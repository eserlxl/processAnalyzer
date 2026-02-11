// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_H
#define ANALYZER_H

#include <string>
#include <string_view> // Re-added
#include <vector>
#include <optional>     // For std::optional
#include <functional>   // For std::function
#include <cstdint>      // For uint32_t
#include <chrono>       // For std::chrono
#include <map>          // For std::map
#include <filesystem>   // Added for std::filesystem::path

#include <system_error> // For std::error_code (optional, could use int errno directly)
#include <generator>    // For std::generator (C++23)
#include <sys/resource.h> // For setpriority, PRIO_PROCESS
#include <sched.h>        // For sched_setaffinity, cpu_set_t
#include <regex>        // For std::regex (C++11)

#include "utils/types.h" // For utils::Result<T> and UtilsError

#include "analyzer/process_model.h"
#include "analyzer/system_model.h"
#include "analyzer/network_model.h"

class ProcessAnalyzer {
public:
    explicit ProcessAnalyzer(std::filesystem::path procPath = "/proc");

    // For testing purposes
    void setProcPath(const std::filesystem::path& newPath) {
        procPath = newPath;
    }

    const std::filesystem::path& getProcPath() const {
        return procPath;
    }
    
    // Core API
    utils::Result<std::vector<int>> getPids() const;
    utils::Result<ProcessInfo> getProcessDetails(int pid) const;
    utils::Result<std::vector<ProcessInfo>> snapshot() const;

    // --- C++23 Lazy Loading API ---
    std::generator<int> streamPids() const;
    std::generator<ProcessInfo> streamProcesses() const;
    std::generator<ProcessInfo> streamQueryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

    // --- New System-wide Statistics API for Iteration 7 ---
    utils::Result<SystemMemoryInfo> getSystemMemoryInfo() const;
    utils::Result<SystemLoadAverage> getSystemLoadAverage() const;
    utils::Result<SystemCpuStats> getSystemCpuStats() const;

    // Filtering API
    
    // New method for general process query with filtering and sorting
    // Contract: Returns processes matching filter, sorted as specified.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<ProcessInfo>> queryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

    // New method to get child processes
    // Contract: Returns a vector of ProcessInfo for direct children of the given PID.
    //           Returns empty vector if no children or PID not found.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<ProcessInfo>> getChildProcesses(int pid) const;

    // New: Get the system's clock tick frequency (HZ)
    // Contract: Returns the system clock ticks per second.
    //           Typically 100 for older systems, 1000 for newer ones, but can vary.
    // Uses std::expected for error reporting.
    static utils::Result<long> getSystemClockTicksPerSecond();

    // New: Calculate CPU usage for a specific process over a given duration.
    // Contract: Returns the CPU usage percentage for the specified PID.
    //           This method will internally take two snapshots (before and after a delay)
    //           and calculate the delta.
    // Parameters:
    //   pid: The process ID to monitor.
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A ProcessCpuUsage object. `cpuPercentage` will be 0 if process not found or no change.
    // Uses std::expected for error reporting.
    utils::Result<ProcessCpuUsage> getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const;

    static utils::Result<long long> getSystemBootTimeUnix(const std::filesystem::path& procPath); // Added static public member for testability

    // New: Calculate CPU usage for all processes over a given duration.
    // Contract: Returns a vector of ProcessCpuUsage objects for all active processes.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A vector of ProcessCpuUsage objects. Processes that exit during the observation
    //          or have no CPU activity will have 0% usage.
    // Uses std::expected for error reporting.
        
    // New: Get the parent process of a given PID.
    // Contract: Returns the ProcessInfo of the parent process.
    // Uses std::expected for error reporting.
    utils::Result<ProcessInfo> getParentProcess(int pid) const;

    // New: Get all descendant processes (children, grandchildren, etc.) of a given PID.
    // Contract: Returns a vector of ProcessInfo for all direct and indirect descendants.
    //           Returns empty vector if no descendants or PID not found.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<ProcessInfo>> getAllDescendantProcesses(int pid) const;

    // New: Retrieve environment variables for a specific process.
    // Contract: Returns a vector of strings, where each string is an "KEY=VALUE" pair.
    //           Returns empty vector if no environment variables are found or inaccessible.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<std::string>> getProcessEnvironment(int pid) const;

    // New for Iteration 9: Process Control/Manipulation (Signal Handling)
    // Contract: Sends the specified signal to the process identified by pid.
    // Parameters:
    //   pid: The process ID to which the signal will be sent.
    //   signal: The signal to send.
    // Uses std::expected for error reporting.
    static utils::Result<void> sendSignal(int pid, ProcessSignal signal);

    // New for Iteration 14: Process Context Information
    utils::Result<std::vector<MemoryMapInfo>> getProcessMemoryMaps(int pid) const;
    utils::Result<ResourceLimitInfo> getProcessResourceLimits(int pid) const;
    utils::Result<CgroupInfo> getProcessCgroupInfo(int pid) const;
    utils::Result<std::vector<OpenFileDescriptorInfo>> getProcessOpenFileDetails(pid_t pid) const;

    // New for Iteration 14: Process Control Capabilities
    static utils::Result<void> setProcessNiceness(int pid, int niceness);
    static utils::Result<void> setProcessCpuAffinity(int pid, const CpuSet& affinity);

    // New for Iteration 14: System-wide Metrics
    utils::Result<PerCpuUsage> getPerCpuUsage(std::chrono::milliseconds durationMs) const;
    utils::Result<std::vector<DiskIoDeviceStats>> getSystemDiskIoStats() const;
    utils::Result<std::vector<NetworkInterfaceStats>> getNetworkInterfaceStats() const;
    utils::Result<SystemActivityStats> getSystemActivityStats() const;

    // New for Iteration 13: Network Activity Monitoring
    // Contract: Returns a vector of NetworkConnection objects for the specified PID.
    //           Parses /proc/[pid]/net/{tcp,tcp6,udp,udp6,unix}
    // Parameters:
    //   pid: The process ID.
    // Returns: A vector of NetworkConnection. Empty if no connections or process not found/inaccessible.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<NetworkConnection>> getNetworkConnections(int pid) const;

    // New for Iteration 9: Disk I/O Rate per Process
    // Contract: Returns the disk I/O rate for the specified PID.
    //           This method will take two snapshots of /proc/[pid]/io (before and after a delay)
    //           and calculate the delta.
    // Parameters:
    //   pid: The process ID to monitor.
    //   durationMs: The duration in milliseconds to observe I/O.
    // Returns: A ProcessDiskIoUsage object. Rates will be 0 if process not found or no change.
    // Uses std::expected for error reporting.
    utils::Result<ProcessDiskIoUsage> getProcessDiskIoUsage(int pid, std::chrono::milliseconds durationMs) const;

    // New for Iteration 9: Calculate disk I/O rates for all processes over a given duration.
    // Contract: Returns a vector of ProcessDiskIoUsage objects for all active processes.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe I/O.
    // Returns: A vector of ProcessDiskIoUsage objects. Processes that exit during observation
    //          or have no I/O will have 0 rates.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<ProcessDiskIoUsage>> getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const;

    // New for Iteration 9: Process Threads Details
    // Contract: Returns a vector of ThreadInfo objects for all threads of the given PID.
    //           Parses /proc/[pid]/task/[tid]/stat and /proc/[pid]/task/[tid]/comm.
    // Parameters:
    //   pid: The process ID.
    // Returns: A vector of ThreadInfo. Empty if process not found, no threads (e.g., defunct), or inaccessible.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<ThreadInfo>> getProcessThreads(int pid) const;

    // New for Iteration 9: System-wide Disk Usage Information
    // Contract: Returns a vector of MountPointInfo for each mounted filesystem.
    //           Uses statfs system call or parses /proc/mounts.
    // Returns: A vector of MountPointInfo. Empty if no mount points are accessible.
    // Uses std::expected for error reporting.
    utils::Result<std::vector<MountPointInfo>> getSystemDiskUsage() const;

    // New for Iteration 9: System Uptime and Kernel Information
    // Contract: Returns a SystemInfo object.
    // Uses std::expected for error reporting.
    
    // New for Iteration 9: Calculate total system CPU usage percentage over a given duration.
    // Contract: Returns the total system CPU usage percentage.
    //           This method will take two snapshots of /proc/stat (before and after a delay)
    //           and calculate the delta for the 'cpu' line.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A SystemCpuUsage object. `cpuPercentage` will be 0 if unable to calculate or no activity.
    // Uses std::expected for error reporting.
    
private:
    std::filesystem::path procPath;
    // Store last known CPU times for delta calculation, accessible across calls if needed for efficiency
    mutable std::map<int, std::pair<long long, long long>> lastCpuTimes; // pid -> {user_ticks, kernel_ticks}
    // Need a way to also store total system CPU time to normalize
    mutable std::optional<long long> lastTotalSystemCpuTimeTicks; // Total system CPU time ticks
    mutable std::map<int, std::pair<long long, long long>> lastIoBytes; // pid -> {read_bytes, write_bytes}
    mutable std::optional<SystemCpuStats> lastSystemCpuStats; // Store for delta calculation
    mutable std::optional<PerCpuUsage> lastPerCpuStats; // Store for delta calculation of per-CPU usage
};

#endif
