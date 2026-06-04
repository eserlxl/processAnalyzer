// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_SYSTEM_MODEL_H
#define ANALYZER_SYSTEM_MODEL_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <cstdint>

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
    unsigned long long guest = 0;
    unsigned long long guestNice = 0;
};

// New for Iteration 14: Per-CPU Usage
struct SingleCpuUsage {
    int cpuId; // Zero-based core index: cpu0 → 0, cpu1 → 1, …; the aggregate "cpu " line is excluded.
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

#endif // ANALYZER_SYSTEM_MODEL_H
