// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <cstdint>
#include <chrono>
#include <map>
#include <filesystem>
#include <system_error>
#include <generator>
#include <sys/resource.h>
#include <sched.h>
#include <regex>

#include "utils/types.h"
#include "analyzer/process_model.h"
#include "analyzer/system_model.h"
#include "analyzer/network_model.h"

class ProcessAnalyzer {
public:
    explicit ProcessAnalyzer(std::filesystem::path procPath = "/proc");

    // For testing purposes
    void setProcPath(const std::filesystem::path& newPath);

    [[nodiscard]] const std::filesystem::path& getProcPath() const;

    //- Core Process Information
    [[nodiscard]] utils::Result<std::vector<int>> getPids() const;
    [[nodiscard]] utils::Result<ProcessInfo> getProcessDetails(int pid) const;
    [[nodiscard]] utils::Result<std::vector<ProcessInfo>> snapshot() const;

    //- Process Hierarchy and Relationships
    [[nodiscard]] utils::Result<ProcessInfo> getParentProcess(int pid) const;
    [[nodiscard]] utils::Result<std::vector<ProcessInfo>> getChildProcesses(int pid) const;
    [[nodiscard]] utils::Result<std::vector<ProcessInfo>> getAllDescendantProcesses(int pid) const;

    //- Detailed Process Context
    [[nodiscard]] utils::Result<std::vector<ThreadInfo>> getProcessThreads(int pid) const;
    [[nodiscard]] utils::Result<std::vector<std::string>> getProcessEnvironment(int pid) const;
    [[nodiscard]] utils::Result<std::vector<MemoryMapInfo>> getProcessMemoryMaps(int pid) const;
    [[nodiscard]] utils::Result<ResourceLimitInfo> getProcessResourceLimits(int pid) const;
    [[nodiscard]] utils::Result<CgroupInfo> getProcessCgroupInfo(int pid) const;
    [[nodiscard]] utils::Result<std::vector<OpenFileDescriptorInfo>> getProcessOpenFileDetails(int pid) const;
    [[nodiscard]] utils::Result<std::vector<NetworkConnection>> getNetworkConnections(int pid) const;

    //- Process Performance Metrics (Self-Contained)
    [[nodiscard]] utils::Result<ProcessCpuUsage> getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const;
    [[nodiscard]] utils::Result<std::vector<ProcessCpuUsage>> getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const;
    [[nodiscard]] utils::Result<ProcessDiskIoUsage> getProcessDiskIoUsage(int pid, std::chrono::milliseconds durationMs) const;
    [[nodiscard]] utils::Result<std::vector<ProcessDiskIoUsage>> getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const;

    //- Process Control & Manipulation
    static utils::Result<void> sendSignal(int pid, ProcessSignal signal);
    static utils::Result<void> setProcessNiceness(int pid, int niceness);
    static utils::Result<void> setProcessCpuAffinity(int pid, const CpuSet& affinity);

    //- System-wide Information & Statistics
    [[nodiscard]] utils::Result<SystemInfo> getSystemInfo() const;
    [[nodiscard]] utils::Result<long long> getSystemBootTimeUnix() const;
    static utils::Result<long> getSystemClockTicksPerSecond();
    [[nodiscard]] utils::Result<SystemMemoryInfo> getSystemMemoryInfo() const;
    [[nodiscard]] utils::Result<SystemLoadAverage> getSystemLoadAverage() const;
    [[nodiscard]] utils::Result<std::vector<MountPointInfo>> getSystemDiskUsage() const;
    [[nodiscard]] utils::Result<std::vector<DiskIoDeviceStats>> getSystemDiskIoStats() const;
    [[nodiscard]] utils::Result<std::vector<NetworkInterfaceStats>> getNetworkInterfaceStats() const;
    [[nodiscard]] utils::Result<SystemActivityStats> getSystemActivityStats() const;

    //- System Performance Metrics (Self-Contained)
    [[nodiscard]] utils::Result<SystemCpuStats> getSystemCpuStats() const;
    [[nodiscard]] utils::Result<SystemCpuUsage> getSystemCpuUsage(std::chrono::milliseconds durationMs) const;
    [[nodiscard]] utils::Result<PerCpuUsage> getPerCpuUsage(std::chrono::milliseconds durationMs) const;

    //- Process Query & Filtering
    [[nodiscard]] utils::Result<std::vector<ProcessInfo>> queryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

    //- C++23 Streaming API (Generators)
    //- WARNING: The returned generator MUST NOT outlive the ProcessAnalyzer instance.
    //- Using the generator after the analyzer is destroyed will result in a dangling reference and undefined behavior.
    [[nodiscard]] std::generator<int> streamPids() const;
    [[nodiscard]] std::generator<ProcessInfo> streamProcesses() const;
    [[nodiscard]] std::generator<ProcessInfo> streamQueryProcesses(
        const ProcessFilter& filter = {},
        ProcessSortField sortBy = ProcessSortField::pid,
        SortOrder sortOrder = SortOrder::asc
    ) const;

private:
    std::filesystem::path procPath;
};
