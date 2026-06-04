
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/types.h"
#include "utils/file.h"
#include "utils/string.h"

#include <algorithm>
#include <string>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <sys/statvfs.h>
#include <cctype>
#include <set>

// Anonymous namespace for helper functions
namespace {

struct RawCpuLine {
    int cpuId = -1;
    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;
};

// Parse all per-CPU lines (cpu0, cpu1, ...) from /proc/stat content.
// Lines starting with "cpu " (aggregate) or non-digit suffix are skipped.
std::vector<RawCpuLine> parsePerCpuLines(const std::string& content) {
    std::vector<RawCpuLine> result;
    std::istringstream iss{content};
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.starts_with("cpu") || line.size() < 4) continue;
        if (!std::isdigit(static_cast<unsigned char>(line[3]))) continue;
        std::istringstream ls(line);
        std::string label;
        RawCpuLine raw;
        if (!(ls >> label >> raw.user >> raw.nice >> raw.system >> raw.idle
                        >> raw.iowait >> raw.irq >> raw.softirq >> raw.steal)) {
            continue;
        }
        raw.cpuId = std::stoi(label.substr(3));
        result.push_back(raw);
    }
    return result;
}

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
utils::Result<long long> ProcessAnalyzer::getSystemBootTimeUnix() const {
    auto statContent = utils::readTextFile((procPath / "stat").string());
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

// Implementation of ProcessAnalyzer::getSystemActivityRates
utils::Result<SystemActivityRates> ProcessAnalyzer::getSystemActivityRates(
    std::chrono::milliseconds duration) const {
    auto snap1 = getSystemActivityStats();
    if (!snap1) return std::unexpected(snap1.error());

    std::this_thread::sleep_for(duration);

    auto snap2 = getSystemActivityStats();
    if (!snap2) return std::unexpected(snap2.error());

    const double secs = static_cast<double>(std::max(duration.count(), decltype(duration.count()){1})) / 1000.0;
    return SystemActivityRates{
        .contextSwitchesPerSec = static_cast<double>(snap2->contextSwitches - snap1->contextSwitches) / secs,
        .interruptsPerSec      = static_cast<double>(snap2->interruptsTotal  - snap1->interruptsTotal)  / secs,
        .processForkRate       = static_cast<double>(snap2->processesForked  - snap1->processesForked)  / secs,
    };
}

// Implementation of ProcessAnalyzer::getSystemCpuStats
utils::Result<SystemCpuStats> ProcessAnalyzer::getSystemCpuStats() const {
    auto content = utils::readTextFile((procPath / "stat").string());
    if (!content) {
        return std::unexpected(content.error());
    }

    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.starts_with("cpu ")) {
            continue;
        }
        SystemCpuStats stats;
        std::string label;
        std::istringstream ls(line);
        if (!(ls >> label >> stats.user >> stats.nice >> stats.system >> stats.idle
                          >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal)) {
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
        }
        ls >> stats.guest >> stats.guestNice; // optional fields; ignore failures
        return stats;
    }

    return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

// Implementation of ProcessAnalyzer::getSystemCpuUsage
utils::Result<SystemCpuUsage> ProcessAnalyzer::getSystemCpuUsage(std::chrono::milliseconds duration) const {
    auto stats1 = getSystemCpuStats();
    if (!stats1) {
        return std::unexpected(stats1.error());
    }

    std::this_thread::sleep_for(duration);

    auto stats2 = getSystemCpuStats();
    if (!stats2) {
        return std::unexpected(stats2.error());
    }

    const auto active1 = stats1->user + stats1->nice + stats1->system
                         + stats1->irq + stats1->softirq + stats1->steal;
    const auto active2 = stats2->user + stats2->nice + stats2->system
                         + stats2->irq + stats2->softirq + stats2->steal;
    const auto total1  = active1 + stats1->idle + stats1->iowait;
    const auto total2  = active2 + stats2->idle + stats2->iowait;

    const auto totalDelta = total2 - total1;
    if (totalDelta == 0ULL) {
        return SystemCpuUsage{0.0};
    }

    const auto activeDelta = static_cast<double>(active2 - active1);
    return SystemCpuUsage{activeDelta / static_cast<double>(totalDelta) * 100.0};
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

// Implementation of ProcessAnalyzer::getSystemDiskIoRates
utils::Result<std::vector<DiskIoDeviceRates>> ProcessAnalyzer::getSystemDiskIoRates(
    std::chrono::milliseconds duration) const {
    auto snap1 = getSystemDiskIoStats();
    if (!snap1) return std::unexpected(snap1.error());

    std::this_thread::sleep_for(duration);

    auto snap2 = getSystemDiskIoStats();
    if (!snap2) return std::unexpected(snap2.error());

    const double secs = static_cast<double>(std::max(duration.count(), decltype(duration.count()){1})) / 1000.0;

    std::unordered_map<std::string, const DiskIoDeviceStats*> map1;
    map1.reserve(snap1->size());
    for (const auto& s : *snap1) {
        map1.emplace(s.deviceName, &s);
    }

    std::vector<DiskIoDeviceRates> rates;
    rates.reserve(snap2->size());
    for (const auto& s2 : *snap2) {
        auto it = map1.find(s2.deviceName);
        if (it == map1.end()) continue;
        const auto& s1 = *it->second;
        DiskIoDeviceRates r;
        r.deviceName          = s2.deviceName;
        r.readsPerSec         = static_cast<double>(s2.readsCompleted  - s1.readsCompleted)  / secs;
        r.writesPerSec        = static_cast<double>(s2.writesCompleted - s1.writesCompleted) / secs;
        r.sectorsReadPerSec   = static_cast<double>(s2.sectorsRead     - s1.sectorsRead)     / secs;
        r.sectorsWrittenPerSec= static_cast<double>(s2.sectorsWritten  - s1.sectorsWritten)  / secs;
        rates.push_back(std::move(r));
    }
    return rates;
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
        "bpf", "autofs", "fusectl", "efivarfs", "configfs",
        // kernel virtual filesystems and snap/container layers
        "binfmt_misc", "overlay", "squashfs", "ramfs",
        // FUSE-based virtual filesystems (portal, gvfs, sshfs, etc.)
        "fuse", "fuseblk"
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
        // Filter fuse.* subtypes (fuse.portal, fuse.gvfsd-fuse, fuse.sshfs, etc.)
        if (mp.filesystemType.starts_with("fuse.")) continue;
        struct statvfs sv{};
        if (statvfs(mp.mountPoint.c_str(), &sv) != 0) continue;
        mp.totalSpaceBytes = static_cast<unsigned long long>(sv.f_blocks) * sv.f_frsize;
        if (mp.totalSpaceBytes == 0) continue;
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

    // Try the canonical path first; fall back to procPath/etc/os-release for test mocks.
    auto osReleaseContent = utils::readTextFile("/etc/os-release");
    if (!osReleaseContent) {
        osReleaseContent = utils::readTextFile((procPath / "etc" / "os-release").string());
    }
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

// Implementation of ProcessAnalyzer::getPerCpuUsage
utils::Result<PerCpuUsage> ProcessAnalyzer::getPerCpuUsage(std::chrono::milliseconds duration) const {
    auto content1 = utils::readTextFile((procPath / "stat").string());
    if (!content1) {
        return std::unexpected(content1.error());
    }

    std::this_thread::sleep_for(duration);

    auto content2 = utils::readTextFile((procPath / "stat").string());
    if (!content2) {
        return std::unexpected(content2.error());
    }

    const auto snap1 = parsePerCpuLines(*content1);
    const auto snap2 = parsePerCpuLines(*content2);

    PerCpuUsage result;
    const auto count = std::min(snap1.size(), snap2.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto& s1 = snap1[i];
        const auto& s2 = snap2[i];
        const auto active1 = s1.user + s1.nice + s1.system + s1.irq + s1.softirq + s1.steal;
        const auto active2 = s2.user + s2.nice + s2.system + s2.irq + s2.softirq + s2.steal;
        const auto total1  = active1 + s1.idle + s1.iowait;
        const auto total2  = active2 + s2.idle + s2.iowait;
        const auto totalDelta = total2 - total1;
        double pct = 0.0;
        if (totalDelta > 0ULL) {
            pct = static_cast<double>(active2 - active1) / static_cast<double>(totalDelta) * 100.0;
        }
        result.cpuUsages.push_back(SingleCpuUsage{.cpuId = s2.cpuId, .cpuPercentage = pct});
    }
    return result;
}
