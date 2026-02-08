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
#include <expected>     // For std::expected (C++23)
#include <system_error> // For std::error_code (optional, could use int errno directly)
#include <generator>    // For std::generator (C++23)
#include <sys/resource.h> // For setpriority, PRIO_PROCESS
#include <sched.h>        // For sched_setaffinity, cpu_set_t
#include <regex>        // For std::regex (C++11)

// New: Define specific error codes for Analyzer
enum class AnalyzerError {
    processNotFound,
    permissionDenied,
    fileNotFound,
    parsingError,
    invalidArgument,
    systemError, // Generic system error with errno
    operationNotSupported, // For features not available on current OS/kernel
    // ... potentially more specific errors
};

// New: Custom error type to carry more info
struct AnalyzerErrorDetail {
    AnalyzerError code;
    std::string message;
    std::optional<int> systemErrno; // Store actual errno if applicable
};

// Enum for common POSIX signals
enum class ProcessSignal {
    sighup = 1,    // Hangup detected on controlling terminal or death of controlling process
    sigint = 2,    // Interrupt from keyboard
    sigquit = 3,   // Quit from keyboard
    sigkill = 9,   // Kill signal (cannot be caught or ignored)
    sigterm = 15,  // Termination signal
    sigstop = 19,  // Stop process (cannot be caught or ignored)
    sigcont = 18,  // Continue process if stopped
    sigusR1 = 10,  // User-defined signal 1
    sigusR2 = 12   // User-defined signal 2
    // ... add more as needed
};

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
    long long startTimeUnix;  // Process start time as Unix timestamp (seconds since epoch) - NEW for Iteration 13
    std::string elapsedTime;  // Formatted string representing uptime, e.g., "01:23:45" or "1d 2h" - NEW for Iteration 13
    std::string executablePath; // Path to the executable file (symlink /proc/<pid>/exe)
    std::string currentWorkingDirectory; // Current working directory (symlink /proc/<pid>/cwd)
    std::vector<std::string> environmentVariables; // Environment variables (from /proc/<pid>/environ)
    long long cpuUserTimeTicks; // User mode CPU time in clock ticks
    long long cpuKernelTimeTicks; // Kernel mode CPU time in clock ticks
    long long ioReadBytes;      // Total bytes read by the process (from /proc/<pid>/io)
    long long ioWriteBytes;     // Total bytes written by the process (from /proc/<pid>/io)
    int priority;               // Process priority (nice value)
    
    // --- New Fields for Iteration 13 ---
    float cpuUsage;             // Percentage of CPU usage.
    float memoryPercentage;     // Percentage of total system memory used by the process (RSS-based).
};

struct ProcessCpuUsage {
    int pid;
    std::string name;
    double cpuPercentage; // CPU usage as a percentage (e.g., 50.5 for 50.5% CPU)
                          // Note: This is an instantaneous/delta percentage between two calls.
};

// --- New Structs for Iteration 7 ---
struct SystemMemoryInfo {
    unsigned long memTotal = 0;       // In kB
    unsigned long memFree = 0;        // In kB
    unsigned long memAvailable = 0;   // In kB
    unsigned long buffers = 0;        // In kB
    unsigned long cached = 0;         // In kB
    unsigned long swapTotal = 0;      // In kB
    unsigned long swapFree = 0;       // In kB
};

struct SystemLoadAverage {
    double oneMin = 0.0;
    double fiveMin = 0.0;
    double fifteenMin = 0.0;
};

struct SystemCpuStats {
    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;
};

// New for Iteration 9: Network Activity Monitoring
// Enum for network protocol families
enum class AddressFamily {
    iPv4,
    iPv6,
    unknown
};

// Enum for socket types
enum class SocketType {
    tcp,
    udp,
    raw,
    UNIX, // Unix Domain Sockets
    unknown
};

struct NetworkConnection {
    std::string protocol;    // e.g., "TCP", "UDP", "TCP6", "UDP6"
    std::string localAddress;  // Local IP address, e.g., "127.0.0.1"
    std::string remoteAddress; // Remote IP address, e.g., "192.168.1.100" or "*"
    uint16_t localPort;        // Local port number
    uint16_t remotePort;       // Remote port number (0 if not connected/LISTEN)
    std::string state;       // Connection state, e.g., "ESTABLISHED", "LISTEN", "TIME_WAIT"
    int inode;               // Socket inode number
    AddressFamily addressFamily; // IPv4 or IPv6
    SocketType socketType;     // TCP or UDP (or RAW/UNIX if expanded)
};

// New for Iteration 9: Disk I/O Rate per Process
struct ProcessDiskIoUsage {
    int pid;
    std::string name;
    double readBytesPerSecond;  // Disk read rate in bytes per second
    double writeBytesPerSecond; // Disk write rate in bytes per second
};

// New for Iteration 9: Process Threads Details
struct ThreadInfo {
    int tid;                // Thread ID (which is also the PID of the kernel's representation of the thread)
    std::string name;       // Thread name (from /proc/[pid]/task/[tid]/comm)
    std::string state;      // Thread state (from /proc/[pid]/task/[tid]/stat)
    long long cpuUserTimeTicks;    // User mode CPU time in clock ticks for this thread
    long long cpuKernelTimeTicks;  // Kernel mode CPU time in clock ticks for this thread
    // ... potentially add more details like priority, context switches if needed
};

// New for Iteration 14: Memory Maps
struct MemoryMapInfo {
    uint64_t startAddress;
    uint64_t endAddress;
    std::string permissions; // e.g., "r-xp"
    uint64_t offset;
    std::string device;      // e.g., "08:01"
    uint64_t inode;
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
    int id;
    std::string controllers; // Comma-separated list, e.g., "cpu,cpuacct"
    std::string path;        // Path within the cgroup hierarchy
};

struct CgroupInfo {
    std::vector<CgroupEntry> entries;
};

// New for Iteration 14: Detailed Open File Descriptors
enum class OpenFileType {
    File,
    Socket,
    Pipe,
    AnonInode,
    Device,
    Other,
    Unknown
};

struct OpenFileDescriptorInfo {
    int fd;
    std::string path; // Target path of the symlink (e.g., filename, socket:[inode])
    OpenFileType type; // Categorized type (File, Socket, Pipe, etc.)
    // Additional info for sockets could be added here later if needed,
    // e.g., referencing NetworkConnection details via inode.
};

// New for Iteration 14: Per-CPU Usage
struct SingleCpuUsage {
    int cpuId; // 0 for total, 1 for cpu1, etc.
    double cpuPercentage; // Usage for this specific CPU
};

struct PerCpuUsage {
    std::vector<SingleCpuUsage> cpuUsages; // Includes total system CPU as cpuId 0
};

// New for Iteration 14: Disk I/O per Device
struct DiskIoDeviceStats {
    std::string deviceName; // e.g., "sda", "nvme0n1"
    uint64_t readsCompleted;
    uint64_t readsMerged;
    uint64_t sectorsRead;
    uint64_t readTimeMs;
    uint64_t writesCompleted;
    uint64_t writesMerged;
    uint64_t sectorsWritten;
    uint64_t writeTimeMs;
    uint64_t ioProgressMs;
    uint64_t ioWeightedTimeMs;
};

// New for Iteration 14: Network Interface Statistics
struct NetworkInterfaceStats {
    std::string interfaceName; // e.g., "eth0", "lo"
    uint64_t rxBytes;
    uint64_t rxPackets;
    uint64_t rxErrors;
    uint64_t rxDropped;
    uint64_t txBytes;
    uint64_t txPackets;
    uint64_t txErrors;
    uint64_t txDropped;
    // ... more fields from /proc/net/dev as needed
};

// New for Iteration 14: Interrupts and Context Switches
struct SystemActivityStats {
    uint64_t interruptsTotal;
    std::map<std::string, uint64_t> interruptsPerCpu; // e.g., "cpu0": 12345
    uint64_t contextSwitches;
    uint64_t processesForked;
    // ... potentially other stats from /proc/stat
};

// New for Iteration 14: CPU Set for affinity
struct CpuSet {
    std::vector<int> cpus; // List of CPU core IDs (0-indexed)
    // Can add utility methods like `has(int cpu_id)`, `add(int cpu_id)`
};

// New for Iteration 9: System-wide Disk Usage Information
struct MountPointInfo {
    std::string mountPoint; // e.g., "/"
    std::string filesystemType; // e.g., "ext4"
    std::string device;     // e.g., "/dev/sda1"
    unsigned long long totalSpaceBytes; // Total size in bytes
    unsigned long long freeSpaceBytes;  // Free size in bytes
    unsigned long long availableSpaceBytes; // Available size for unprivileged users in bytes
};

// New for Iteration 9: System Uptime and Kernel Information
struct SystemInfo {
    std::chrono::seconds uptime;      // System uptime
    std::string kernelVersion;        // e.g., "Linux version 5.15.0-76-generic"
    std::string osName;               // e.g., "Ubuntu" (derived from /etc/os-release or similar)
    std::string hostname;             // System hostname
};

// New for Iteration 9: System-wide CPU Usage Percentage (Over Duration)
struct SystemCpuUsage {
    double cpuPercentage; // Total system CPU usage as a percentage (e.g., 50.5 for 50.5% CPU)
                          // Note: This is a delta percentage between two calls, normalized over all cores.
};

// Predicate for filtering processes
using ProcessPredicate = std::function<bool(const ProcessInfo&)>;

// New structures and enums for Iteration 3 filtering and sorting
struct ProcessFilter {
    std::optional<std::string> nameContains; // Existing: Simple substring match
    std::optional<std::regex> nameRegex;     // New: Regex for process name
    std::optional<std::string> userFilter;
    std::optional<char> stateFilter;

    std::optional<int> minThreads;
    std::optional<int> maxThreads;
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

    std::optional<double> minCpuUsage;          // New: Filter by CPU usage percentage
    std::optional<double> maxCpuUsage;          // New
    std::optional<double> minMemoryPercentage;  // New: Filter by memory usage percentage
    std::optional<double> maxMemoryPercentage;  // New

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

enum class ProcessSortField {
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
    priority,
    cpuUsage,         // NEW for Iteration 13
    memoryPercentage  // NEW for Iteration 13
};

enum class SortOrder {
    asc,
    desc
};

class ProcessAnalyzer {
public:
    explicit ProcessAnalyzer(std::string_view procPath = "/proc");
    
    // Core API
    std::expected<std::vector<int>, AnalyzerErrorDetail> getPids() const;
    std::expected<ProcessInfo, AnalyzerErrorDetail> getProcessDetails(int pid) const;
    std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> snapshot() const;

    // --- C++23 Lazy Loading API ---
    std::generator<int> streamPids() const;
    std::generator<ProcessInfo> streamProcesses() const;
    std::generator<ProcessInfo> streamQueryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

    // --- New System-wide Statistics API for Iteration 7 ---
    std::expected<SystemMemoryInfo, AnalyzerErrorDetail> getSystemMemoryInfo() const;
    std::expected<SystemLoadAverage, AnalyzerErrorDetail> getSystemLoadAverage() const;
    std::expected<SystemCpuStats, AnalyzerErrorDetail> getSystemCpuStats() const;

    // Filtering API
    std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> findProcesses(const ProcessPredicate& predicate) const;

    // New method for general process query with filtering and sorting
    // Contract: Returns processes matching filter, sorted as specified.
    // Uses std::expected for error reporting.
    std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> queryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

    // New method to get child processes
    // Contract: Returns a vector of ProcessInfo for direct children of the given PID.
    //           Returns empty vector if no children or PID not found.
    // Uses std::expected for error reporting.
    std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> getChildProcesses(int pid) const;

    // New: Get the system's clock tick frequency (HZ)
    // Contract: Returns the system clock ticks per second.
    //           Typically 100 for older systems, 1000 for newer ones, but can vary.
    // Uses std::expected for error reporting.
    static std::expected<long, AnalyzerErrorDetail> getSystemClockTicksPerSecond();

    // New: Calculate CPU usage for a specific process over a given duration.
    // Contract: Returns the CPU usage percentage for the specified PID.
    //           This method will internally take two snapshots (before and after a delay)
    //           and calculate the delta.
    // Parameters:
    //   pid: The process ID to monitor.
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A ProcessCpuUsage object. `cpuPercentage` will be 0 if process not found or no change.
    // Uses std::expected for error reporting.
    std::expected<ProcessCpuUsage, AnalyzerErrorDetail> getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const;

    // New: Calculate CPU usage for all processes over a given duration.
    // Contract: Returns a vector of ProcessCpuUsage objects for all active processes.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A vector of ProcessCpuUsage objects. Processes that exit during the observation
    //          or have no CPU activity will have 0% usage.
    // Uses std::expected for error reporting.
    std::expected<std::vector<ProcessCpuUsage>, AnalyzerErrorDetail> getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const;
    
    // New: Get the parent process of a given PID.
    // Contract: Returns the ProcessInfo of the parent process.
    // Uses std::expected for error reporting.
    std::expected<ProcessInfo, AnalyzerErrorDetail> getParentProcess(int pid) const;

    // New: Get all descendant processes (children, grandchildren, etc.) of a given PID.
    // Contract: Returns a vector of ProcessInfo for all direct and indirect descendants.
    //           Returns empty vector if no descendants or PID not found.
    // Uses std::expected for error reporting.
    std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> getAllDescendantProcesses(int pid) const;

    // New: Retrieve environment variables for a specific process.
    // Contract: Returns a vector of strings, where each string is an "KEY=VALUE" pair.
    //           Returns empty vector if no environment variables are found or inaccessible.
    // Uses std::expected for error reporting.
    std::expected<std::vector<std::string>, AnalyzerErrorDetail> getProcessEnvironment(int pid) const;

    // New for Iteration 9: Process Control/Manipulation (Signal Handling)
    // Contract: Sends the specified signal to the process identified by pid.
    // Parameters:
    //   pid: The process ID to which the signal will be sent.
    //   signal: The signal to send.
    // Uses std::expected for error reporting.
    static std::expected<void, AnalyzerErrorDetail> sendSignal(int pid, ProcessSignal signal);

    // New for Iteration 14: Process Context Information
    std::expected<std::vector<MemoryMapInfo>, AnalyzerErrorDetail> getProcessMemoryMaps(int pid) const;
    std::expected<ResourceLimitInfo, AnalyzerErrorDetail> getProcessResourceLimits(int pid) const;
    std::expected<CgroupInfo, AnalyzerErrorDetail> getProcessCgroupInfo(int pid) const;
    std::expected<std::vector<OpenFileDescriptorInfo>, AnalyzerErrorDetail> getProcessOpenFileDetails(pid_t pid) const;

    // New for Iteration 14: Process Control Capabilities
    static std::expected<void, AnalyzerErrorDetail> setProcessNiceness(int pid, int niceness);
    static std::expected<void, AnalyzerErrorDetail> setProcessCpuAffinity(int pid, const CpuSet& affinity);

    // New for Iteration 14: System-wide Metrics
    std::expected<PerCpuUsage, AnalyzerErrorDetail> getPerCpuUsage(std::chrono::milliseconds durationMs) const;
    std::expected<std::vector<DiskIoDeviceStats>, AnalyzerErrorDetail> getSystemDiskIoStats() const;
    std::expected<std::vector<NetworkInterfaceStats>, AnalyzerErrorDetail> getNetworkInterfaceStats() const;
    std::expected<SystemActivityStats, AnalyzerErrorDetail> getSystemActivityStats() const;

    // New for Iteration 13: Network Activity Monitoring
    // Contract: Returns a vector of NetworkConnection objects for the specified PID.
    //           Parses /proc/[pid]/net/{tcp,tcp6,udp,udp6,unix}
    // Parameters:
    //   pid: The process ID.
    // Returns: A vector of NetworkConnection. Empty if no connections or process not found/inaccessible.
    // Uses std::expected for error reporting.
    std::expected<std::vector<NetworkConnection>, AnalyzerErrorDetail> getNetworkConnections(int pid) const;

    // New for Iteration 9: Disk I/O Rate per Process
    // Contract: Returns the disk I/O rate for the specified PID.
    //           This method will take two snapshots of /proc/[pid]/io (before and after a delay)
    //           and calculate the delta.
    // Parameters:
    //   pid: The process ID to monitor.
    //   durationMs: The duration in milliseconds to observe I/O.
    // Returns: A ProcessDiskIoUsage object. Rates will be 0 if process not found or no change.
    // Uses std::expected for error reporting.
    std::expected<ProcessDiskIoUsage, AnalyzerErrorDetail> getProcessDiskIoUsage(int pid, std::chrono::milliseconds durationMs) const;

    // New for Iteration 9: Calculate disk I/O rates for all processes over a given duration.
    // Contract: Returns a vector of ProcessDiskIoUsage objects for all active processes.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe I/O.
    // Returns: A vector of ProcessDiskIoUsage objects. Processes that exit during observation
    //          or have no I/O will have 0 rates.
    // Uses std::expected for error reporting.
    std::expected<std::vector<ProcessDiskIoUsage>, AnalyzerErrorDetail> getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const;

    // New for Iteration 9: Process Threads Details
    // Contract: Returns a vector of ThreadInfo objects for all threads of the given PID.
    //           Parses /proc/[pid]/task/[tid]/stat and /proc/[pid]/task/[tid]/comm.
    // Parameters:
    //   pid: The process ID.
    // Returns: A vector of ThreadInfo. Empty if process not found, no threads (e.g., defunct), or inaccessible.
    // Uses std::expected for error reporting.
    std::expected<std::vector<ThreadInfo>, AnalyzerErrorDetail> getProcessThreads(int pid) const;

    // New for Iteration 9: System-wide Disk Usage Information
    // Contract: Returns a vector of MountPointInfo for each mounted filesystem.
    //           Uses statfs system call or parses /proc/mounts.
    // Returns: A vector of MountPointInfo. Empty if no mount points are accessible.
    // Uses std::expected for error reporting.
    std::expected<std::vector<MountPointInfo>, AnalyzerErrorDetail> getSystemDiskUsage() const;

    // New for Iteration 9: System Uptime and Kernel Information
    // Contract: Returns a SystemInfo object.
    // Uses std::expected for error reporting.
    std::expected<SystemInfo, AnalyzerErrorDetail> getSystemInfo() const;

    // New for Iteration 9: Calculate total system CPU usage percentage over a given duration.
    // Contract: Returns the total system CPU usage percentage.
    //           This method will take two snapshots of /proc/stat (before and after a delay)
    //           and calculate the delta for the 'cpu' line.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A SystemCpuUsage object. `cpuPercentage` will be 0 if unable to calculate or no activity.
    // Uses std::expected for error reporting.
    std::expected<SystemCpuUsage, AnalyzerErrorDetail> getSystemCpuUsage(std::chrono::milliseconds durationMs) const;

private:
    std::string procPath;
    // Store last known CPU times for delta calculation, accessible across calls if needed for efficiency
    mutable std::map<int, std::pair<long long, long long>> lastCpuTimes; // pid -> {user_ticks, kernel_ticks}
    // Need a way to also store total system CPU time to normalize
    mutable std::optional<long long> lastTotalSystemCpuTimeTicks; // Total system CPU time ticks
    mutable std::map<int, std::pair<long long, long long>> lastIoBytes; // pid -> {read_bytes, write_bytes}
    mutable std::optional<SystemCpuStats> lastSystemCpuStats; // Store for delta calculation
    mutable std::optional<PerCpuUsage> lastPerCpuStats; // Store for delta calculation of per-CPU usage
};

#endif
