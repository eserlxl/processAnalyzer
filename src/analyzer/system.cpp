
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/types.h"
#include "utils/file.h"
#include "utils/string.h"

#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/statvfs.h>
#include <set>

// Anonymous namespace for helper functions
namespace {

// Helper to parse /proc/stat for system boot time
utils::Result<long long> parseSystemBootTime(std::string_view statContent) {
    std::istringstream iss{std::string(statContent)};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("btime ")) {
            std::istringstream lineStream(line);
            std::string label; // "btime"
            long long bootTimeUnix;
            if (!(lineStream >> label >> bootTimeUnix)) {
                return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
            }
            return bootTimeUnix;
        }
    }
    return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

} // anonymous namespace

// Implementation of ProcessAnalyzer::getSystemBootTimeUnix
utils::Result<long long> ProcessAnalyzer::getSystemBootTimeUnix() {
    auto statContent = utils::readTextFile("/proc/stat");
    if (!statContent) {
        return std::unexpected(statContent.error());
    }
    return parseSystemBootTime(*statContent);
}

// Implementation of ProcessAnalyzer::getSystemClockTicksPerSecond
utils::Result<long> ProcessAnalyzer::getSystemClockTicksPerSecond() {
    long ticks = sysconf(_SC_CLK_TCK);
    if (ticks == -1) {
        return std::unexpected(std::error_code(errno, std::system_category()));
    }
    return ticks;
}

// Implementation of ProcessAnalyzer::getSystemMemoryInfo
utils::Result<SystemMemoryInfo> ProcessAnalyzer::getSystemMemoryInfo() const {
    auto content = utils::readTextFile((procPath / "meminfo").string());
    if (!content) {
        return std::unexpected(content.error());
    }

    SystemMemoryInfo info;
    // Each /proc/meminfo line is "<Label>: <value> kB"; pull out the value.
    auto readKb = [](const std::string& line, unsigned long& out) {
        std::istringstream ls(line);
        std::string label;
        unsigned long value = 0;
        std::string unit;
        if (ls >> label >> value >> unit) {
            out = value;
        }
    };

    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("MemTotal:")) {
            readKb(line, info.memTotal);
        } else if (line.starts_with("MemFree:")) {
            readKb(line, info.memFree);
        } else if (line.starts_with("MemAvailable:")) {
            readKb(line, info.memAvailable);
        } else if (line.starts_with("Buffers:")) {
            readKb(line, info.buffers);
        } else if (line.starts_with("Cached:")) { // not "SwapCached:"
            readKb(line, info.cached);
        } else if (line.starts_with("SwapTotal:")) {
            readKb(line, info.swapTotal);
        } else if (line.starts_with("SwapFree:")) {
            readKb(line, info.swapFree);
        }
    }
    return info;
}

// Implementation of ProcessAnalyzer::getSystemLoadAverage
utils::Result<SystemLoadAverage> ProcessAnalyzer::getSystemLoadAverage() const {
    auto content = utils::readTextFile((procPath / "loadavg").string());
    if (!content) {
        return std::unexpected(content.error());
    }
    SystemLoadAverage avg;
    std::istringstream iss{*content};
    if (!(iss >> avg.oneMin >> avg.fiveMin >> avg.fifteenMin)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    return avg;
}

// Implementation of ProcessAnalyzer::getSystemActivityStats
utils::Result<SystemActivityStats> ProcessAnalyzer::getSystemActivityStats() const {
    auto content = utils::readTextFile((procPath / "stat").string());
    if (!content) {
        return std::unexpected(content.error());
    }
    SystemActivityStats stats{};
    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("intr ")) {
            std::istringstream ls(line);
            std::string label;
            ls >> label >> stats.interruptsTotal;
            uint64_t perCpu = 0;
            int cpuIdx = 0;
            while (ls >> perCpu) {
                stats.interruptsPerCpu["cpu" + std::to_string(cpuIdx++)] = perCpu;
            }
        } else if (line.starts_with("ctxt ")) {
            std::istringstream ls(line);
            std::string label;
            ls >> label >> stats.contextSwitches;
        } else if (line.starts_with("processes ")) {
            std::istringstream ls(line);
            std::string label;
            ls >> label >> stats.processesForked;
        }
    }
    return stats;
}

// Implementation of ProcessAnalyzer::getSystemDiskIoStats
utils::Result<std::vector<DiskIoDeviceStats>> ProcessAnalyzer::getSystemDiskIoStats() const {
    auto content = utils::readTextFile((procPath / "diskstats").string());
    if (!content) {
        return std::unexpected(content.error());
    }
    std::vector<DiskIoDeviceStats> result;
    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        int major = 0;
        int minor = 0;
        DiskIoDeviceStats dev{};
        if (!(ls >> major >> minor >> dev.deviceName
                 >> dev.readsCompleted >> dev.readsMerged >> dev.sectorsRead >> dev.readTimeMs
                 >> dev.writesCompleted >> dev.writesMerged >> dev.sectorsWritten >> dev.writeTimeMs
                 >> dev.ioProgressMs >> dev.ioWeightedTimeMs)) {
            continue;
        }
        result.push_back(std::move(dev));
    }
    return result;
}

// Implementation of ProcessAnalyzer::getSystemDiskUsage
utils::Result<std::vector<MountPointInfo>> ProcessAnalyzer::getSystemDiskUsage() const {
    auto content = utils::readTextFile((procPath / "mounts").string());
    if (!content) {
        return std::unexpected(content.error());
    }
    static const std::set<std::string> pseudoFsTypes{
        "proc", "sysfs", "devtmpfs", "cgroup", "cgroup2", "tmpfs", "devpts",
        "hugetlbfs", "mqueue", "debugfs", "tracefs", "securityfs", "pstore",
        "bpf", "autofs", "fusectl", "efivarfs", "configfs"
    };
    std::vector<MountPointInfo> result;
    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        MountPointInfo mp;
        std::string opts;
        std::string freq;
        std::string passno;
        if (!(ls >> mp.device >> mp.mountPoint >> mp.filesystemType >> opts >> freq >> passno)) {
            continue;
        }
        if (pseudoFsTypes.contains(mp.filesystemType)) continue;
        struct statvfs sv{};
        if (statvfs(mp.mountPoint.c_str(), &sv) != 0) continue;
        mp.totalSpaceBytes = static_cast<unsigned long long>(sv.f_blocks) * sv.f_frsize;
        mp.freeSpaceBytes = static_cast<unsigned long long>(sv.f_bfree) * sv.f_frsize;
        mp.availableSpaceBytes = static_cast<unsigned long long>(sv.f_bavail) * sv.f_frsize;
        result.push_back(std::move(mp));
    }
    return result;
}

// Implementation of ProcessAnalyzer::getSystemInfo
utils::Result<SystemInfo> ProcessAnalyzer::getSystemInfo() const {
    auto uptimeContent = utils::readTextFile((procPath / "uptime").string());
    if (!uptimeContent) {
        return std::unexpected(uptimeContent.error());
    }
    auto versionContent = utils::readTextFile((procPath / "version").string());
    if (!versionContent) {
        return std::unexpected(versionContent.error());
    }

    SystemInfo info;

    // Parse uptime: first double is seconds
    {
        std::istringstream ls(*uptimeContent);
        double uptimeSecs = 0.0;
        if (ls >> uptimeSecs) {
            info.uptime = std::chrono::seconds(static_cast<long long>(uptimeSecs));
        }
    }

    // Kernel version: entire first line of /proc/version
    {
        std::istringstream ls(*versionContent);
        std::getline(ls, info.kernelVersion);
        info.kernelVersion = utils::trim(info.kernelVersion);
    }

    // Hostname: /proc/sys/kernel/hostname
    auto hostnameContent = utils::readTextFile((procPath / "sys" / "kernel" / "hostname").string());
    if (hostnameContent) {
        info.hostname = utils::trim(*hostnameContent);
    }

    // osName: /proc/etc/os-release (via procPath for testability) then fallback
    auto osReleaseContent = utils::readTextFile((procPath / "etc" / "os-release").string());
    if (osReleaseContent) {
        std::istringstream ls(*osReleaseContent);
        std::string oLine;
        while (std::getline(ls, oLine)) {
            constexpr std::size_t namePrefixLen = 5; // "NAME="
            if (oLine.starts_with("NAME=")) {
                info.osName = oLine.substr(namePrefixLen);
                // Strip surrounding quotes
                if (!info.osName.empty() && info.osName.front() == '"') {
                    info.osName = info.osName.substr(1);
                }
                if (!info.osName.empty() && info.osName.back() == '"') {
                    info.osName.pop_back();
                }
                break;
            }
        }
    }
    if (info.osName.empty()) {
        info.osName = "Linux";
    }

    return info;
}

