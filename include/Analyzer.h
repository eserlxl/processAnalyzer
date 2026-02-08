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
    AddressFamily family;
    SocketType type;
    std::string localAddress;
    uint16_t localPort;
    std::optional<std::string> remoteAddress; // For connected sockets
    std::optional<uint16_t> remotePort;     // For connected sockets
    std::string state;                      // e.g., "LISTEN", "ESTABLISHED", "CLOSE_WAIT"
    std::optional<int> inode;                 // Inode associated with the socket
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

    // --- New Fields for Iteration 7 ---
    std::optional<pid_t> ppidFilter;

    // New for Iteration 9: Optional custom predicate for more advanced filtering.
    // Contract: If provided, this predicate will be applied IN ADDITION to other filter fields.
    //           Only processes for which this predicate returns true will be included.
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
    vm,
    threads,
    cwd,
    cpuUserTime,
    cpuKernelTime,
    ioReadBytes,
    ioWriteBytes,
    priority
};

enum class SortOrder {
    asc,
    desc
};

class ProcessAnalyzer {
public:
    explicit ProcessAnalyzer(std::string_view procPath = "/proc");
    
    // Core API
    std::vector<int> getPids() const;
    std::expected<ProcessInfo, AnalyzerErrorDetail> getProcessDetails(int pid) const;
    std::vector<ProcessInfo> snapshot() const;

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
    std::vector<ProcessInfo> findProcesses(const ProcessPredicate& predicate) const;

    // New method for general process query with filtering and sorting
    // Contract: Returns processes matching filter, sorted as specified.
    // Throws: std::runtime_error if procPath is invalid or permissions issues.
    std::vector<ProcessInfo> queryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

    // New method to get child processes
    // Contract: Returns a vector of ProcessInfo for direct children of the given PID.
    //           Returns empty vector if no children or PID not found.
    // Throws: std::runtime_error for permissions issues.
    std::vector<ProcessInfo> getChildProcesses(int pid) const;

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

    // New for Iteration 7: Get open file descriptors for a process
    // Contract: Returns a map of file descriptors to their target paths.
    //           Returns an empty map if the process does not exist or has no FDs.
    // Throws: std::runtime_error for permissions issues reading the /proc/[pid]/fd directory.
    std::map<int, std::string> getOpenFileDescriptors(pid_t pid) const;

    // New for Iteration 9: Process Control/Manipulation (Signal Handling)
    // Contract: Sends the specified signal to the process identified by pid.
    // Parameters:
    //   pid: The process ID to which the signal will be sent.
    //   signal: The signal to send.
    // Returns: true if the signal was sent successfully, false otherwise (e.g., process not found, permission denied).
    // Throws: std::runtime_error for unexpected system errors.
    static bool sendSignal(int pid, ProcessSignal signal);

    // New for Iteration 9: Network Activity Monitoring
    // Contract: Returns a vector of NetworkConnection objects for the specified PID.
    //           Parses /proc/[pid]/net/{tcp,tcp6,udp,udp6,unix}
    // Parameters:
    //   pid: The process ID.
    // Returns: A vector of NetworkConnection. Empty if no connections or process not found/inaccessible.
    // Throws: std::runtime_error for permissions issues or parsing errors.
    std::vector<NetworkConnection> getProcessNetworkConnections(int pid) const;

    // New for Iteration 9: Disk I/O Rate per Process
    // Contract: Returns the disk I/O rate for the specified PID.
    //           This method will take two snapshots of /proc/[pid]/io (before and after a delay)
    //           and calculate the delta.
    // Parameters:
    //   pid: The process ID to monitor.
    //   durationMs: The duration in milliseconds to observe I/O.
    // Returns: A ProcessDiskIoUsage object. Rates will be 0 if process not found or no change.
    // Throws: std::runtime_error if unable to get process details or if sleep fails.
    std::optional<ProcessDiskIoUsage> getProcessDiskIoUsage(int pid, std::chrono::milliseconds durationMs) const;

    // New for Iteration 9: Calculate disk I/O rates for all processes over a given duration.
    // Contract: Returns a vector of ProcessDiskIoUsage objects for all active processes.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe I/O.
    // Returns: A vector of ProcessDiskIoUsage objects. Processes that exit during observation
    //          or have no I/O will have 0 rates.
    // Throws: std::runtime_error if unable to get process details or if sleep fails.
    std::vector<ProcessDiskIoUsage> getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const;

    // New for Iteration 9: Process Threads Details
    // Contract: Returns a vector of ThreadInfo objects for all threads of the given PID.
    //           Parses /proc/[pid]/task/[tid]/stat and /proc/[pid]/task/[tid]/comm.
    // Parameters:
    //   pid: The process ID.
    // Returns: A vector of ThreadInfo. Empty if process not found, no threads (e.g., defunct), or inaccessible.
    // Throws: std::runtime_error for permissions issues or parsing errors.
    std::vector<ThreadInfo> getProcessThreads(int pid) const;

    // New for Iteration 9: System-wide Disk Usage Information
    // Contract: Returns a vector of MountPointInfo for each mounted filesystem.
    //           Uses statfs system call or parses /proc/mounts.
    // Returns: A vector of MountPointInfo. Empty if no mount points are accessible.
    // Throws: std::runtime_error for system call failures or parsing errors.
    std::vector<MountPointInfo> getSystemDiskUsage() const;

    // New for Iteration 9: System Uptime and Kernel Information
    // Contract: Returns a SystemInfo object.
    // Throws: std::runtime_error if /proc/uptime, /proc/version, or hostname cannot be read.
    std::optional<SystemInfo> getSystemInfo() const;

    // New for Iteration 9: Calculate total system CPU usage percentage over a given duration.
    // Contract: Returns the total system CPU usage percentage.
    //           This method will take two snapshots of /proc/stat (before and after a delay)
    //           and calculate the delta for the 'cpu' line.
    // Parameters:
    //   durationMs: The duration in milliseconds to observe CPU usage.
    // Returns: A SystemCpuUsage object. `cpuPercentage` will be 0 if unable to calculate or no activity.
    // Throws: std::runtime_error if unable to read /proc/stat or if sleep fails.
    std::optional<SystemCpuUsage> getSystemCpuUsage(std::chrono::milliseconds durationMs) const;

private:
    std::string procPath;
    // Store last known CPU times for delta calculation, accessible across calls if needed for efficiency
    mutable std::map<int, std::pair<long long, long long>> lastCpuTimes; // pid -> {user_ticks, kernel_ticks}
    // Need a way to also store total system CPU time to normalize
    mutable std::optional<long long> lastTotalSystemCpuTimeTicks; // Total system CPU time ticks
    mutable std::map<int, std::pair<long long, long long>> lastIoBytes; // pid -> {read_bytes, write_bytes}
    mutable std::optional<SystemCpuStats> lastSystemCpuStats; // Store for delta calculation
};

#endif
