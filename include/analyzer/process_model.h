// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_PROCESS_MODEL_H
#define ANALYZER_PROCESS_MODEL_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <regex>
#include <functional>

struct ProcessInfo {
    pid_t pid = 0;
    pid_t ppid = 0;
    uid_t uid = 0;
    std::string username = "Unknown";
    std::string name = "Unknown";
    std::string state = "?";
    long long residentMemory = 0; // in KB (formerly memory_usage)
    long long virtualMemory = 0;  // in KB
    long threadCount = 0;
    std::string cmdline;

    // --- New Fields for Iteration 5 ---
    long long startTimeTicks = 0; // Process start time in clock ticks since system boot
    long long startTimeUnix = 0;  // Process start time as Unix timestamp (seconds since epoch) - NEW for Iteration 13
    std::string elapsedTime = "N/A";  // Formatted string representing uptime, e.g., "01:23:45" or "1d 2h" - NEW for Iteration 13
    std::string executablePath; // Path to the executable file (symlink /proc/<pid>/exe)
    std::string currentWorkingDirectory; // Current working directory (symlink /proc/<pid>/cwd)
    std::vector<std::string> environmentVariables; // Environment variables (from /proc/<pid>/environ)
    long long cpuUserTimeTicks = 0; // User mode CPU time in clock ticks
    long long cpuKernelTimeTicks = 0; // Kernel mode CPU time in clock ticks
    long long ioReadBytes = 0;      // Total bytes read by the process (from /proc/<pid>/io)
    long long ioWriteBytes = 0;     // Total bytes written by the process (from /proc/<pid>/io)
    int priority = 0;               // Process priority (nice value)
};

// New for Iteration 9: Process Threads Details
struct ThreadInfo {
    pid_t tid = 0;                // Thread ID (which is also the PID of the kernel's representation of the thread)
    std::string name;       // Thread name (from /proc/[pid]/task/[tid]/comm)
    std::string state = "?";      // Thread state (from /proc/[pid]/task/[tid]/stat)
    long long cpuUserTimeTicks = 0;    // User mode CPU time in clock ticks for this thread
    long long cpuKernelTimeTicks = 0;  // Kernel mode CPU time in clock ticks for this thread
    // ... potentially add more details like priority, context switches if needed
};

// New for Iteration 14: Memory Maps
struct MemoryMapInfo {
    uint64_t startAddress = 0;
    uint64_t endAddress = 0;
    std::string permissions; // e.g., "r-xp"
    uint64_t offset = 0;
    std::string device;      // e.g., "08:01"
    uint64_t inode = 0;
    std::string pathname;    // e.g., "/usr/bin/ls" or "[stack]"
};

// New for Iteration 14: Resource Limits
struct ResourceLimit {
    std::string resource;
    std::string softLimit; // "unlimited" or numerical value
    std::string hardLimit; // "unlimited" or numerical value
    std::string units;     // e.g., "bytes", "seconds"
};

struct ResourceLimitInfo {
    std::vector<ResourceLimit> limits;
};

// New for Iteration 14: Cgroup Information
struct CgroupEntry {
    int id = 0;
    std::string controllers; // Comma-separated list, e.g., "cpu,cpuacct"
    std::string path;        // Path within the cgroup hierarchy
};

struct CgroupInfo {
    std::vector<CgroupEntry> entries;
};

// New for Iteration 14: Detailed Open File Descriptors
enum class OpenFileType : std::uint8_t {
    file,
    socket,
    pipe,
    anonInode,
    device,
    other,
    unknown
};

struct OpenFileDescriptorInfo {
    int fd = -1;
    std::string path; // Target path of the symlink (e.g., filename, socket:[inode])
    OpenFileType type = OpenFileType::unknown; // Categorized type (File, Socket, Pipe, etc.)
    // Additional info for sockets could be added here later if needed,
    // e.g., referencing NetworkConnection details via inode.
};

// Predicate for filtering processes
using ProcessPredicate = std::function<bool(const ProcessInfo&)>;

// New structures and enums for Iteration 3 filtering and sorting
struct ProcessFilter {
    std::optional<std::string> nameContains; // Existing: Simple substring match
    std::optional<std::regex> nameRegex;     // New: Regex for process name
    std::optional<std::string> userFilter;
    std::optional<char> stateFilter;

    std::optional<long> minThreads;
    std::optional<long> maxThreads;
    std::optional<long long> minResidentMemoryKB;
    std::optional<long long> maxResidentMemoryKB;
    std::optional<long long> minVirtualMemoryKB;
    std::optional<long long> maxVirtualMemoryKB;

    std::optional<std::string> cmdlineContains; // Existing
    std::optional<std::regex> cmdlineRegex;    // New: Regex for command line
    std::optional<std::string> executablePathContains;
    std::optional<std::regex> executablePathRegex; // New: Regex for executable path

    std::optional<uint32_t> uidFilter;
    std::optional<int> minPriority;
    std::optional<int> maxPriority;
    std::optional<pid_t> ppidFilter;

    // New: Filter processes with network connections matching criteria
    struct NetworkFilterCriteria {
        std::optional<uint16_t> localPort;
        std::optional<uint16_t> remotePort;
        std::optional<std::string> remoteAddressContains; // Substring match
        std::optional<std::regex> remoteAddressRegex;    // Regex match
        std::optional<std::string> protocol; // "TCP", "UDP", etc.
        std::optional<std::string> state;    // "LISTEN", "ESTABLISHED", etc.
    };
    std::optional<NetworkFilterCriteria> networkConnectionFilter;

    std::optional<ProcessPredicate> customPredicate; 
};

enum class ProcessSortField : std::uint8_t {
    pid,
    ppid,
    rss,
    vmsize,
    startTime,
    cpuTime, // New for Iteration 7
    name,
    executablePath,
    cmdline,
    uid,
    user,
    state,

    threads,
    cwd,
    cpuUserTime,
    cpuKernelTime,
    ioReadBytes,
    ioWriteBytes,
    priority
};

enum class SortOrder : std::uint8_t {
    asc,
    desc
};

#endif // ANALYZER_PROCESS_MODEL_H
