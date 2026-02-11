// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/core.h"

#include <algorithm>
#include <arpa/inet.h>
#include <bit>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstring>
#include <dirent.h>
#include <filesystem>
#include <map>
#include <netinet/in.h>
#include <pwd.h>
#include <ranges>
#include <regex>
#include <sched.h>
#include <set>
#include <sstream>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {
    constexpr double msInSecond = 1000.0;
    constexpr long defaultSystemClockTicks = 100;
    constexpr size_t pwBufSize = 1024;
    constexpr size_t hostnameBufSize = 256;
    
    constexpr std::string_view anyIpV4AddrPort = "00000000:0000";
    constexpr std::string_view anyIpV6AddrPort = "00000000000000000000000000000000:0000";
    constexpr int hexBase = 16;
    constexpr int decimalBase = 10;
    constexpr size_t ipv4HexLen = 8;
    constexpr size_t ipv6HexLen = 32;
    constexpr size_t ipv6ChunkLen = 8;
    constexpr size_t ipv6ChunkCount = 4;

    // Stat field indices from /proc/[pid]/stat
    constexpr int statFieldPpidIndex = 1;
    constexpr int statFieldUserTimeIndex = 11;
    constexpr int statFieldKernelTimeIndex = 12;
    constexpr int statFieldNiceIndex = 16;
    constexpr int statFieldStartTimeIndex = 19;
    constexpr int statFieldMinSize = 20;

    // /proc/[pid]/io prefix lengths
    constexpr size_t readBytesPrefixLen = 11;
    constexpr size_t writeBytesPrefixLen = 12;
    constexpr size_t rcharPrefixLen = 6;
    constexpr size_t wcharPrefixLen = 6;

    // /proc/[pid]/task/[tid]/stat field indices
    constexpr int threadStatFieldMinSize = 13;
    constexpr int threadStatFieldUserTimeIndex = 11;
    constexpr int threadStatFieldKernelTimeIndex = 12;

    utils::Result<long long> getTotalSystemCpuTimeTicks(const ::std::filesystem::path& procPath) {
        auto stats = ProcessAnalyzer(procPath).getSystemCpuStats();
        if(stats) {
            unsigned long long totalTicks = stats->user + stats->nice + stats->system + stats->idle + 
                                            stats->iowait + stats->irq + stats->softirq + stats->steal;
            return static_cast<long long>(totalTicks);
        }
        return std::unexpected(stats.error());
    }

    bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter) {
        if (filter.nameContains && process.name.find(*filter.nameContains) == std::string::npos) return false;
        if (filter.nameRegex && !std::regex_search(process.name, *filter.nameRegex)) return false;
        if (filter.userFilter && process.username != *filter.userFilter) return false;
        if (filter.stateFilter && (process.state.empty() || process.state[0] != *filter.stateFilter)) return false;
        if (filter.minThreads && process.threadCount < *filter.minThreads) return false;
        if (filter.maxThreads && process.threadCount > *filter.maxThreads) return false;
        if (filter.minResidentMemoryKB && process.residentMemory < *filter.minResidentMemoryKB) return false;
        if (filter.maxResidentMemoryKB && process.residentMemory > *filter.maxResidentMemoryKB) return false;
        if (filter.minVirtualMemoryKB && process.virtualMemory < *filter.minVirtualMemoryKB) return false;
        if (filter.maxVirtualMemoryKB && process.virtualMemory > *filter.maxVirtualMemoryKB) return false;
        if (filter.cmdlineContains && process.cmdline.find(*filter.cmdlineContains) == std::string::npos) return false;
        if (filter.cmdlineRegex && !std::regex_search(process.cmdline, *filter.cmdlineRegex)) return false;
        if (filter.executablePathContains && process.executablePath.find(*filter.executablePathContains) == std::string::npos) return false;
        if (filter.executablePathRegex && !std::regex_search(process.executablePath, *filter.executablePathRegex)) return false;
        if (filter.uidFilter && process.uid != *filter.uidFilter) return false;
        if (filter.minPriority && process.priority < *filter.minPriority) return false;
        if (filter.maxPriority && process.priority > *filter.maxPriority) return false;
        if (filter.ppidFilter && process.ppid != *filter.ppidFilter) return false;
        if (filter.minCpuUsage && process.cpuUsage < *filter.minCpuUsage) return false;
        if (filter.maxCpuUsage && process.cpuUsage > *filter.maxCpuUsage) return false;
        if (filter.minMemoryPercentage && process.memoryPercentage < *filter.minMemoryPercentage) return false;
        if (filter.maxMemoryPercentage && process.memoryPercentage > *filter.maxMemoryPercentage) return false;
        if (filter.customPredicate && !(*filter.customPredicate)(process)) return false;
        return true;
    }

    auto processSortPredicate = [](const ProcessInfo& a, const ProcessInfo& b, ProcessSortField sortBy, SortOrder sortOrder) {
        auto compare = [&](const ProcessInfo& lhs, const ProcessInfo& rhs) -> int {
            switch (sortBy) {
                case ProcessSortField::pid: if (lhs.pid != rhs.pid) return lhs.pid < rhs.pid ? -1 : 1; break;
                case ProcessSortField::ppid: if (lhs.ppid != rhs.ppid) return lhs.ppid < rhs.ppid ? -1 : 1; break;
                case ProcessSortField::uid: if (lhs.uid != rhs.uid) return lhs.uid < rhs.uid ? -1 : 1; break;
                case ProcessSortField::user: if (lhs.username != rhs.username) return lhs.username < rhs.username ? -1 : 1; break;
                case ProcessSortField::name: if (lhs.name != rhs.name) return lhs.name < rhs.name ? -1 : 1; break;
                case ProcessSortField::state: if (lhs.state != rhs.state) return lhs.state < rhs.state ? -1 : 1; break;
                case ProcessSortField::rss: if (lhs.residentMemory != rhs.residentMemory) return lhs.residentMemory < rhs.residentMemory ? -1 : 1; break;
                case ProcessSortField::vmsize: if (lhs.virtualMemory != rhs.virtualMemory) return lhs.virtualMemory < rhs.virtualMemory ? -1 : 1; break;
                case ProcessSortField::threads: if (lhs.threadCount != rhs.threadCount) return lhs.threadCount < rhs.threadCount ? -1 : 1; break;
                case ProcessSortField::startTime: if (lhs.startTimeTicks != rhs.startTimeTicks) return lhs.startTimeTicks < rhs.startTimeTicks ? -1 : 1; break;
                case ProcessSortField::executablePath: if (lhs.executablePath != rhs.executablePath) return lhs.executablePath < rhs.executablePath ? -1 : 1; break;
                case ProcessSortField::cmdline: if (lhs.cmdline != rhs.cmdline) return lhs.cmdline < rhs.cmdline ? -1 : 1; break;
                case ProcessSortField::cpuTime: {
                    auto leftTime = lhs.cpuUserTimeTicks + lhs.cpuKernelTimeTicks;
                    auto rightTime = rhs.cpuUserTimeTicks + rhs.cpuKernelTimeTicks;
                    if (leftTime != rightTime) return leftTime < rightTime ? -1 : 1;
                    break;
                }
                case ProcessSortField::cwd: if (lhs.currentWorkingDirectory != rhs.currentWorkingDirectory) return lhs.currentWorkingDirectory < rhs.currentWorkingDirectory ? -1 : 1; break;
                case ProcessSortField::cpuUserTime: if (lhs.cpuUserTimeTicks != rhs.cpuUserTimeTicks) return lhs.cpuUserTimeTicks < rhs.cpuUserTimeTicks ? -1 : 1; break;
                case ProcessSortField::cpuKernelTime: if (lhs.cpuKernelTimeTicks != rhs.cpuKernelTimeTicks) return lhs.cpuKernelTimeTicks < rhs.cpuKernelTimeTicks ? -1 : 1; break;
                case ProcessSortField::ioReadBytes: if (lhs.ioReadBytes != rhs.ioReadBytes) return lhs.ioReadBytes < rhs.ioReadBytes ? -1 : 1; break;
                case ProcessSortField::ioWriteBytes: if (lhs.ioWriteBytes != rhs.ioWriteBytes) return lhs.ioWriteBytes < rhs.ioWriteBytes ? -1 : 1; break;
                case ProcessSortField::priority: if (lhs.priority != rhs.priority) return lhs.priority < rhs.priority ? -1 : 1; break;
                case ProcessSortField::cpuUsage: if (lhs.cpuUsage != rhs.cpuUsage) return lhs.cpuUsage < rhs.cpuUsage ? -1 : 1; break;
                case ProcessSortField::memoryPercentage: if (lhs.memoryPercentage != rhs.memoryPercentage) return lhs.memoryPercentage < rhs.memoryPercentage ? -1 : 1; break;
                default: if (lhs.pid != rhs.pid) return lhs.pid < rhs.pid ? -1 : 1; break;
            }
            return 0;
        };

        int cmp = compare(a, b);
        if (cmp == 0) return a.pid < b.pid;
        
        return sortOrder == SortOrder::asc ? (cmp < 0) : (cmp > 0);
    };

    utils::Result<void> checkPidPathExistsAndPermissions(const fs::path& procPath, pid_t pid) {
        fs::path pidPath = procPath / std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
        }
        try {
            fs::directory_iterator testIter(pidPath);
        } catch (const fs::filesystem_error&) {
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        }
        return {};
    }

    utils::Result<std::vector<std::string>> readProcessEnvironmentVars(const fs::path& procPath, pid_t pid) {
        std::vector<std::string> env;
        fs::path environPath = procPath / std::to_string(pid) / "environ";

        auto environContentOpt = utils::readTextFile(environPath.string());
        if (!environContentOpt) {
            auto check = checkPidPathExistsAndPermissions(procPath, pid);
            if (!check) return std::unexpected(check.error());
            return env;
        }

        std::string_view content = *environContentOpt;
        size_t start = 0;
        while(start < content.size()) {
            size_t end = content.find('\0', start);
            if (end == std::string_view::npos) break;
            env.emplace_back(content.substr(start, end - start));
            start = end + 1;
        }
        return env;
    }

} // namespace

ProcessAnalyzer::ProcessAnalyzer(std::filesystem::path procPath) : procPath(std::move(procPath)) {}

void ProcessAnalyzer::setProcPath(const std::filesystem::path& newPath) {
    procPath = newPath;
}

const std::filesystem::path& ProcessAnalyzer::getProcPath() const {
    return procPath;
}

utils::Result<long long> ProcessAnalyzer::getSystemBootTimeUnix() const {
    std::filesystem::path uptimePath = procPath / "uptime";
    auto contentOpt = utils::readTextFile(uptimePath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    std::stringstream ss(*contentOpt);
    double uptimeSeconds;
    ss >> uptimeSeconds;

    if (ss.fail()) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    auto now = std::chrono::system_clock::now();
    long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    long long bootTimeUnix = currentTimeUnix - static_cast<long long>(uptimeSeconds);

    return bootTimeUnix;
}

utils::Result<SystemInfo> ProcessAnalyzer::getSystemInfo() const {
    SystemInfo sysInfo;

    // Get Kernel Version
    fs::path versionPath = procPath / "version";
    if (auto versionContent = utils::readTextFile(versionPath.string())) {
        sysInfo.kernelVersion = utils::trim(*versionContent);
    } else {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    // Get Uptime
    fs::path uptimePath = procPath / "uptime";
    if (auto uptimeContent = utils::readTextFile(uptimePath.string())) {
        std::stringstream ss(*uptimeContent);
        double uptimeSeconds;
        ss >> uptimeSeconds;
        sysInfo.uptime = std::chrono::seconds(static_cast<long long>(uptimeSeconds));
    } else {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    // Get Hostname
    std::array<char, hostnameBufSize> hostnameBuf;
    if (gethostname(hostnameBuf.data(), hostnameBuf.size()) == 0) {
        sysInfo.hostname = hostnameBuf.data();
    } else {
        sysInfo.hostname = "unknown";
    }

    return sysInfo;
}


utils::Result<std::vector<ProcessCpuUsage>> ProcessAnalyzer::getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshotResult = snapshot();
    if (!initialSnapshotResult) {
        return std::unexpected(initialSnapshotResult.error());
    }
    
    auto initialTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!initialTotalSystemTicksResult) {
        return std::unexpected(initialTotalSystemTicksResult.error());
    }

    std::this_thread::sleep_for(durationMs);

    auto finalSnapshotResult = snapshot();
    if (!finalSnapshotResult) {
        return std::unexpected(finalSnapshotResult.error());
    }
    
    auto finalTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!finalTotalSystemTicksResult) {
        return std::unexpected(finalTotalSystemTicksResult.error());
    }
    
    long long totalSystemTicksDelta = *finalTotalSystemTicksResult - *initialTotalSystemTicksResult;

    std::map<int, ProcessInfo> initialSnapshotMap;
    for(const auto& info : *initialSnapshotResult) {
        initialSnapshotMap[info.pid] = info;
    }

    std::vector<ProcessCpuUsage> results;
    if (totalSystemTicksDelta <= 0) {
        return results;
    }

    for(const auto& finalInfo : *finalSnapshotResult) {
        auto it = initialSnapshotMap.find(finalInfo.pid);
        if (it != initialSnapshotMap.end()) {
            const auto& initialInfo = it->second;
            long long processCpuTicksDelta = (finalInfo.cpuUserTimeTicks + finalInfo.cpuKernelTimeTicks) - (initialInfo.cpuUserTimeTicks + initialInfo.cpuKernelTimeTicks);
            results.push_back({
                .pid = finalInfo.pid,
                .name = finalInfo.name,
                .cpuPercentage = 100.0 * static_cast<double>(processCpuTicksDelta) / static_cast<double>(totalSystemTicksDelta)
            });
        }
    }
    return results;
}

utils::Result<SystemCpuUsage> ProcessAnalyzer::getSystemCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialStatsResult = getSystemCpuStats();
    if (!initialStatsResult) {
        return std::unexpected(initialStatsResult.error());
    }

    std::this_thread::sleep_for(durationMs);

    auto finalStatsResult = getSystemCpuStats();
    if (!finalStatsResult) {
        return std::unexpected(finalStatsResult.error());
    }

    auto& initialStats = *initialStatsResult;
    auto& finalStats = *finalStatsResult;

    unsigned long long initialTotal = initialStats.user + initialStats.nice + initialStats.system + initialStats.idle + initialStats.iowait + initialStats.irq + initialStats.softirq + initialStats.steal;
    unsigned long long finalTotal = finalStats.user + finalStats.nice + finalStats.system + finalStats.idle + finalStats.iowait + finalStats.irq + finalStats.softirq + finalStats.steal;

    unsigned long long totalDelta = finalTotal - initialTotal;
    unsigned long long idleDelta = finalStats.idle - initialStats.idle;

    double cpuPct = 0.0;
    if (totalDelta > 0) {
        cpuPct = 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta));
    }
    
    return SystemCpuUsage { .cpuPercentage = cpuPct };
}

utils::Result<std::vector<int>> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (utils::isInteger(filename)) {
                    if (auto parsedPid = utils::parseIntegerNoThrow<int>(filename, decimalBase)) {
                        pids.push_back(*parsedPid);
                    }
                }
            }
        }
    } catch (const fs::filesystem_error&) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return pids;
}

std::generator<int> ProcessAnalyzer::streamPids() const {
    if (!fs::exists(procPath)) {
        co_return; 
    }

    for (const auto& entry : fs::directory_iterator(procPath)) {
        if (entry.is_directory()) {
            std::string filename = entry.path().filename().string();
            if (utils::isInteger(filename)) {
                if (auto parsedPid = utils::parseIntegerNoThrow<int>(filename, decimalBase)) {
                    co_yield *parsedPid;
                }
            }
        }
    }
}

utils::Result<SystemMemoryInfo> ProcessAnalyzer::getSystemMemoryInfo() const {
    fs::path meminfoPath = procPath / "meminfo";
    auto contentOpt = utils::readTextFile(meminfoPath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    SystemMemoryInfo memInfo;
    std::stringstream ss(*contentOpt);
    std::string line;
    while (std::getline(ss, line)) {
        std::string key;
        unsigned long value = 0;
        std::stringstream lineSs(line);
        lineSs >> key >> value;
        if(key.empty()) continue;
        if (key.back() == ':') {
            key.pop_back();
        }

        if (key == "MemTotal") memInfo.memTotal = value;
        else if (key == "MemFree") memInfo.memFree = value;
        else if (key == "MemAvailable") memInfo.memAvailable = value;
        else if (key == "Buffers") memInfo.buffers = value;
        else if (key == "Cached") memInfo.cached = value;
        else if (key == "SwapTotal") memInfo.swapTotal = value;
        else if (key == "SwapFree") memInfo.swapFree = value;
    }
    return memInfo;
}

utils::Result<SystemLoadAverage> ProcessAnalyzer::getSystemLoadAverage() const {
    fs::path loadavgPath = procPath / "loadavg";
    auto contentOpt = utils::readTextFile(loadavgPath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    SystemLoadAverage loadAvg;
    std::stringstream ss(*contentOpt);
    if (!(ss >> loadAvg.oneMin >> loadAvg.fiveMin >> loadAvg.fifteenMin)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    return loadAvg;
}

utils::Result<SystemCpuStats> ProcessAnalyzer::getSystemCpuStats() const {
    fs::path statPath = procPath / "stat";
    auto contentOpt = utils::readTextFile(statPath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line);

    if (line.starts_with("cpu ")) {
        SystemCpuStats stats;
        std::stringstream lineSs(line);
        std::string cpuLabel;
        if (!(lineSs >> cpuLabel >> stats.user >> stats.nice >> stats.system >> stats.idle
                     >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal)) {
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
        }
        return stats;
    }

    return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

utils::Result<long> ProcessAnalyzer::getSystemClockTicksPerSecond() {
    long ticks = sysconf(_SC_CLK_TCK);
    if (ticks < 0) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
    return ticks;
}

utils::Result<std::vector<MountPointInfo>> ProcessAnalyzer::getSystemDiskUsage() const {
    std::vector<MountPointInfo> mounts;
    fs::path mountsPath = procPath / "mounts";
    auto contentOpt = utils::readTextFile(mountsPath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        std::stringstream lineSs(line);
        std::string device;
        std::string mountPoint;
        std::string filesystemType;
        lineSs >> device >> mountPoint >> filesystemType;

        struct statvfs vfs;
        if (statvfs(mountPoint.c_str(), &vfs) == 0) {
            mounts.push_back({
                .mountPoint = mountPoint,
                .filesystemType = filesystemType,
                .device = device,
                .totalSpaceBytes = (unsigned long long)vfs.f_blocks * vfs.f_frsize,
                .freeSpaceBytes = (unsigned long long)vfs.f_bfree * vfs.f_frsize,
                .availableSpaceBytes = (unsigned long long)vfs.f_bavail * vfs.f_frsize
            });
        }
    }
    return mounts;
}

utils::Result<PerCpuUsage> ProcessAnalyzer::getPerCpuUsage(std::chrono::milliseconds durationMs) const {
    struct CpuSnapshot {
        unsigned long long total;
        unsigned long long idle;
    };
    
    auto getSnapshots = [&](const fs::path& path) -> std::vector<CpuSnapshot> {
        std::vector<CpuSnapshot> snapshots;
        auto contentOpt = utils::readTextFile(path.string());
        if (!contentOpt) return snapshots;

        std::stringstream ss(*contentOpt);
        std::string line;
        while(std::getline(ss, line)) {
             if (line.starts_with("cpu")) {
                std::stringstream lineSs(line);
                std::string label;
                lineSs >> label;
                
                unsigned long long user = 0;
                unsigned long long nice = 0;
                unsigned long long system = 0;
                unsigned long long idle = 0;
                unsigned long long iowait = 0;
                unsigned long long irq = 0;
                unsigned long long softirq = 0;
                unsigned long long steal = 0;
                lineSs >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
                 
                snapshots.push_back({
                    .total = user + nice + system + idle + iowait + irq + softirq + steal,
                    .idle = idle
                });
             }
        }
        return snapshots;
    };

    fs::path statPath = procPath / "stat";
    auto initialSnapshots = getSnapshots(statPath);
    if (initialSnapshots.empty()) return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));

    std::this_thread::sleep_for(durationMs);

    auto finalSnapshots = getSnapshots(statPath);
    if (finalSnapshots.empty()) return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));

    PerCpuUsage usage;
    for (size_t i = 0; i < initialSnapshots.size() && i < finalSnapshots.size(); ++i) {
        auto totalDelta = finalSnapshots[i].total - initialSnapshots[i].total;
        auto idleDelta = finalSnapshots[i].idle - initialSnapshots[i].idle;
        
        double pct = 0.0;
        if (totalDelta > 0) {
            pct = 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta));
        }
        usage.cpuUsages.push_back({static_cast<int>(i), pct});
    }
    return usage;
}

utils::Result<std::vector<DiskIoDeviceStats>> ProcessAnalyzer::getSystemDiskIoStats() const {
    std::vector<DiskIoDeviceStats> stats;
    fs::path diskStatsPath = procPath / "diskstats";
    auto contentOpt = utils::readTextFile(diskStatsPath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }
    
    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        if (utils::trim(line).empty()) continue;
        std::stringstream lineSs(line);
        int major = 0;
        int minor = 0;
        std::string deviceName;
        if (!(lineSs >> major >> minor >> deviceName)) continue;
        
        DiskIoDeviceStats stat;
        stat.deviceName = deviceName;
        if (!(lineSs >> stat.readsCompleted >> stat.readsMerged >> stat.sectorsRead >> stat.readTimeMs
                     >> stat.writesCompleted >> stat.writesMerged >> stat.sectorsWritten >> stat.writeTimeMs
                     >> stat.ioProgressMs >> stat.ioWeightedTimeMs)) {
            continue;
        }
        stats.push_back(stat);
    }
    if (stats.empty()) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    return stats;
}

utils::Result<SystemActivityStats> ProcessAnalyzer::getSystemActivityStats() const {
    SystemActivityStats stats = {};

    fs::path statPath = procPath / "stat";
    auto contentOpt = utils::readTextFile(statPath.string());
    if (!contentOpt) {
         return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        if (line.starts_with("intr ")) {
            std::stringstream lineSs(line);
            std::string label;
            lineSs >> label >> stats.interruptsTotal;
        } else if (line.starts_with("ctxt ")) {
             std::stringstream lineSs(line);
             std::string label;
             lineSs >> label >> stats.contextSwitches;
        } else if (line.starts_with("processes ")) {
             std::stringstream lineSs(line);
             std::string label;
             lineSs >> label >> stats.processesForked;
        }
    }
    return stats;
}

utils::Result<ProcessInfo> ProcessAnalyzer::getProcessDetails(pid_t pid) const {
    ProcessInfo info;
    info.pid = pid;

    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());
    
    fs::path pidPath = procPath / std::to_string(pid);

    if (auto statusContentOpt = utils::readTextFile((pidPath / "status").string())) {
        for (const auto& line : utils::split(*statusContentOpt, '\n')) {
            auto colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string key = utils::trim(line.substr(0, colonPos));
            std::string value = utils::trim(line.substr(colonPos + 1));
            std::ranges::replace(value, '\t', ' ');

            if (key == "Name") info.name = value;
            else if (key == "State") info.state = value;
            else if (key == "VmSize") (void)utils::tryParse(utils::split(value, ' ')[0], info.virtualMemory);
            else if (key == "VmRSS") (void)utils::tryParse(utils::split(value, ' ')[0], info.residentMemory);
            else if (key == "PPid") (void)utils::tryParse(value, info.ppid);
            else if (key == "Uid") (void)utils::tryParse(utils::split(value, ' ')[0], info.uid);
            else if (key == "Threads") (void)utils::tryParse(value, info.threadCount);
        }
    } else {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    if (auto statContentOpt = utils::readTextFile((pidPath / "stat").string())) {
        const std::string& content = *statContentOpt;
        size_t lastRParen = content.rfind(')');
        if (lastRParen != std::string::npos) {
            std::vector<std::string> statFields = utils::split(utils::trim(content.substr(lastRParen + 1)), ' ', true);
            if (statFields.size() > statFieldStartTimeIndex) {
                (void)utils::tryParse(statFields[statFieldPpidIndex], info.ppid);
                (void)utils::tryParse(statFields[statFieldUserTimeIndex], info.cpuUserTimeTicks);
                (void)utils::tryParse(statFields[statFieldKernelTimeIndex], info.cpuKernelTimeTicks);
                long niceVal = 0;
                (void)utils::tryParse(statFields[statFieldNiceIndex], niceVal);
                info.priority = static_cast<int>(niceVal);
                (void)utils::tryParse(statFields[statFieldStartTimeIndex], info.startTimeTicks);
            }
        }
    }

    if (auto systemBootTimeUnixResult = getSystemBootTimeUnix()) {
        if (auto ticksResult = getSystemClockTicksPerSecond()) {
            long long processStartTimeSec = info.startTimeTicks / *ticksResult;
            info.startTimeUnix = *systemBootTimeUnixResult + processStartTimeSec;
            
            auto now = std::chrono::system_clock::now();
            long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            info.elapsedTime = utils::formatElapsedTime(currentTimeUnix - info.startTimeUnix).value_or("N/A");
        }
    }

    if (auto ioContentOpt = utils::readTextFile((pidPath / "io").string())) {
        for (const auto& line : utils::split(*ioContentOpt, '\n')) {
            if (line.starts_with("read_bytes:")) std::stringstream(line.substr(readBytesPrefixLen)) >> info.ioReadBytes;
            else if (line.starts_with("write_bytes:")) std::stringstream(line.substr(writeBytesPrefixLen)) >> info.ioWriteBytes;
            else if (line.starts_with("rchar:")) std::stringstream(line.substr(rcharPrefixLen)) >> info.ioReadBytes;
            else if (line.starts_with("wchar:")) std::stringstream(line.substr(wcharPrefixLen)) >> info.ioWriteBytes;
        }
    }

    std::array<char, pwBufSize> pwBuf;
    struct passwd pwd;
    struct passwd *result = nullptr;
    if (getpwuid_r(info.uid, &pwd, pwBuf.data(), pwBuf.size(), &result) == 0 && result) {
        info.username = result->pw_name;
    }

    if (auto cmdlineContentOpt = utils::readTextFile((pidPath / "cmdline").string())) {
        std::string rawCmdline = *cmdlineContentOpt;
        std::ranges::replace(rawCmdline, '\0', ' ');
        if (!rawCmdline.empty() && rawCmdline.back() == ' ') rawCmdline.pop_back();
        info.cmdline = rawCmdline;
    } else {
        info.cmdline = "[unreadable]";
    }
    
    try { info.executablePath = fs::read_symlink(pidPath / "exe").string(); } catch (...) { info.executablePath = "[unreadable]"; }
    try { info.currentWorkingDirectory = fs::read_symlink(pidPath / "cwd").string(); } catch (...) { info.currentWorkingDirectory = "[unreadable]"; }

    if (auto envResult = readProcessEnvironmentVars(procPath, pid)) {
        info.environmentVariables = *envResult;
    }
    return info;
}


utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::snapshot() const {
    std::map<int, ProcessInfo> processMap;

    for(const auto& pid : getPids().value_or(std::vector<int>{})) {
        if(auto details = getProcessDetails(pid)) {
            processMap[pid] = *details;
        }
    }
    
    if (auto systemMemoryInfo = getSystemMemoryInfo(); systemMemoryInfo && systemMemoryInfo->memTotal > 0) {
        for (auto& [_, info] : processMap) {
            info.memoryPercentage = (static_cast<float>(info.residentMemory) / static_cast<float>(systemMemoryInfo->memTotal)) * 100.0F;
        }
    }

    std::vector<ProcessInfo> results;
    results.reserve(processMap.size());
    for (const auto& [_, info] : processMap) {
        results.push_back(info);
    }
    return results;
}


utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::queryProcesses(const ProcessFilter& filter, ProcessSortField sortBy, SortOrder sortOrder) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) return std::unexpected(allProcessesResult.error());
    
    std::vector<ProcessInfo> filteredProcesses;
    std::ranges::copy_if(*allProcessesResult, std::back_inserter(filteredProcesses),
        [&](const ProcessInfo& process) { return matchesFilter(process, filter); });

    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) { return processSortPredicate(a, b, sortBy, sortOrder); });

    return filteredProcesses;
}

std::generator<ProcessInfo> ProcessAnalyzer::streamProcesses() const {
    for (int pid : streamPids()) {
        if (auto details = getProcessDetails(pid)) {
            co_yield *details;
        }
    }
}

std::generator<ProcessInfo> ProcessAnalyzer::streamQueryProcesses(const ProcessFilter& filter, ProcessSortField sortBy, SortOrder sortOrder) const {
    std::vector<ProcessInfo> filteredProcesses;
    for (const auto& process : streamProcesses()) {
         if (matchesFilter(process, filter)) {
             filteredProcesses.push_back(process);
         }
    }
    
    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) { return processSortPredicate(a, b, sortBy, sortOrder); });

    for(auto& process : filteredProcesses){
        co_yield process;
    }
}


utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::getChildProcesses(int pid) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) return std::unexpected(allProcessesResult.error());
    
    std::vector<ProcessInfo> children;
    for (const auto& process : *allProcessesResult) {
        if (process.ppid == pid) children.push_back(process);
    }
    return children;
}

utils::Result<ProcessCpuUsage> ProcessAnalyzer::getProcessCpuUsage(pid_t pid, std::chrono::milliseconds durationMs) const {
    auto initialDetailsResult = getProcessDetails(pid);
    if (!initialDetailsResult) return std::unexpected(initialDetailsResult.error());
    
    auto initialTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!initialTotalSystemTicksResult) return std::unexpected(initialTotalSystemTicksResult.error());

    std::this_thread::sleep_for(durationMs);

    auto finalDetailsResult = getProcessDetails(pid);
    if (!finalDetailsResult) return std::unexpected(finalDetailsResult.error());
    
    auto finalTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!finalTotalSystemTicksResult) return std::unexpected(finalTotalSystemTicksResult.error());

    long long processCpuTicksDelta = (finalDetailsResult->cpuUserTimeTicks + finalDetailsResult->cpuKernelTimeTicks) - (initialDetailsResult->cpuUserTimeTicks + initialDetailsResult->cpuKernelTimeTicks);
    long long totalSystemTicksDelta = *finalTotalSystemTicksResult - *initialTotalSystemTicksResult;

    return ProcessCpuUsage {
        .pid = pid,
        .name = finalDetailsResult->name,
        .cpuPercentage = (totalSystemTicksDelta > 0) ? 100.0 * static_cast<double>(processCpuTicksDelta) / static_cast<double>(totalSystemTicksDelta) : 0.0
    };
}

utils::Result<ProcessInfo> ProcessAnalyzer::getParentProcess(pid_t pid) const {
    auto detailsResult = getProcessDetails(pid);
    if (!detailsResult) return std::unexpected(detailsResult.error());
    if (detailsResult->ppid == 0) return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
    return getProcessDetails(detailsResult->ppid);
}

utils::Result<std::vector<ProcessInfo>> ProcessAnalyzer::getAllDescendantProcesses(pid_t pid) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) return std::unexpected(allProcessesResult.error());

    std::vector<ProcessInfo> descendants;
    std::vector<int> toVisit {pid};
    std::map<int, std::vector<ProcessInfo>> parentToChildren;
    for (const auto& p : *allProcessesResult) {
        parentToChildren[p.ppid].push_back(p);
    }
    
    size_t head = 0;
    while(head < toVisit.size()) {
        int currentPid = toVisit[head++];
        if (parentToChildren.contains(currentPid)) {
            for (const auto& child : parentToChildren.at(currentPid)) {
                descendants.push_back(child);
                toVisit.push_back(child.pid);
            }
        }
    }
    return descendants;
}

utils::Result<std::vector<std::string>> ProcessAnalyzer::getProcessEnvironment(pid_t pid) const {
    return readProcessEnvironmentVars(procPath, pid);
}

utils::Result<void> ProcessAnalyzer::sendSignal(int pid, ProcessSignal signal) {
    if (::kill(pid, static_cast<int>(signal)) == 0) return {};
    
    utils::UtilsError errCode = utils::UtilsError::analyzerSystemError;
    if (errno == ESRCH) errCode = utils::UtilsError::analyzerProcessNotFound;
    else if (errno == EPERM) errCode = utils::UtilsError::analyzerPermissionDenied;
    return std::unexpected(utils::make_error_code(errCode));
}

utils::Result<std::vector<ThreadInfo>> ProcessAnalyzer::getProcessThreads(pid_t pid) const {
    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());
    
    std::vector<ThreadInfo> threads;
    fs::path taskPath = procPath / std::to_string(pid) / "task";
    if (!fs::exists(taskPath) || !fs::is_directory(taskPath)) return threads;

    try {
        for (const auto& entry : fs::directory_iterator(taskPath)) {
            if (!entry.is_directory()) continue;
            auto tid = utils::parseIntegerNoThrow<int>(entry.path().filename().string(), decimalBase);
            if (!tid) continue;
            
            ThreadInfo thread;
            thread.tid = *tid;

            if (auto commContent = utils::readTextFile((entry.path() / "comm").string())) {
                thread.name = utils::trim(*commContent);
            }

            if (auto statContentOpt = utils::readTextFile((entry.path() / "stat").string())) {
                const std::string& content = *statContentOpt;
                auto lastParen = content.rfind(')');
                if (lastParen != std::string::npos) {
                    std::vector<std::string> statFields = utils::split(utils::trim(content.substr(lastParen + 1)), ' ', true);
                    if (!statFields.empty() && !statFields[0].empty()) thread.state = statFields[0][0];
                    if (statFields.size() > threadStatFieldKernelTimeIndex) {
                        (void)utils::tryParse(statFields[threadStatFieldUserTimeIndex], thread.cpuUserTimeTicks);
                        (void)utils::tryParse(statFields[threadStatFieldKernelTimeIndex], thread.cpuKernelTimeTicks);
                    }
                }
            }
            threads.push_back(thread);
        }
    } catch (const fs::filesystem_error&) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return threads;
}

utils::Result<ProcessDiskIoUsage> ProcessAnalyzer::getProcessDiskIoUsage(pid_t pid, std::chrono::milliseconds durationMs) const {
    auto initialDetailsResult = getProcessDetails(pid);
    if (!initialDetailsResult) return std::unexpected(initialDetailsResult.error());
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(durationMs);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    auto finalDetailsResult = getProcessDetails(pid);
    if (!finalDetailsResult) return std::unexpected(finalDetailsResult.error());

    double durationSec = std::chrono::duration<double>(endTime - startTime).count();
    double readBytesPerSec = (durationSec > 0) ? static_cast<double>(finalDetailsResult->ioReadBytes - initialDetailsResult->ioReadBytes) / durationSec : 0.0;
    double writeBytesPerSec = (durationSec > 0) ? static_cast<double>(finalDetailsResult->ioWriteBytes - initialDetailsResult->ioWriteBytes) / durationSec : 0.0;

    return ProcessDiskIoUsage {
        .pid = pid,
        .name = finalDetailsResult->name,
        .readBytesPerSecond = std::max(0.0, readBytesPerSec),
        .writeBytesPerSecond = std::max(0.0, writeBytesPerSec)
    };
}

utils::Result<std::vector<ProcessDiskIoUsage>> ProcessAnalyzer::getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshotResult = snapshot();
    if (!initialSnapshotResult) return std::unexpected(initialSnapshotResult.error());
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(durationMs);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto finalSnapshotResult = snapshot();
    if (!finalSnapshotResult) return std::unexpected(finalSnapshotResult.error());

    std::map<int, ProcessInfo> finalSnapshotMap;
    for(const auto& info : *finalSnapshotResult) {
        finalSnapshotMap[info.pid] = info;
    }

    std::vector<ProcessDiskIoUsage> results;
    double durationSec = std::chrono::duration<double>(endTime - startTime).count();
    if (durationSec <= 0) return results;

    for(const auto& initialInfo : *initialSnapshotResult) {
        auto it = finalSnapshotMap.find(initialInfo.pid);
        if (it != finalSnapshotMap.end()) {
            results.push_back({
                .pid = it->second.pid,
                .name = it->second.name,
                .readBytesPerSecond = static_cast<double>(it->second.ioReadBytes - initialInfo.ioReadBytes) / durationSec,
                .writeBytesPerSecond = static_cast<double>(it->second.ioWriteBytes - initialInfo.ioWriteBytes) / durationSec
            });
        }
    }
    return results;
}

utils::Result<std::vector<OpenFileDescriptorInfo>> ProcessAnalyzer::getProcessOpenFileDetails(pid_t pid) const {
    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    std::vector<OpenFileDescriptorInfo> openFds;
    fs::path fdPath = procPath / std::to_string(pid) / "fd";

    try {
        for (const auto& entry : fs::directory_iterator(fdPath)) {
            if (!entry.is_symlink()) continue;
            auto fd = utils::parseIntegerNoThrow<int>(entry.path().filename().string(), decimalBase);
            if (!fd) continue;

            OpenFileDescriptorInfo info;
            info.fd = *fd;

            try { info.path = fs::read_symlink(entry.path()).string(); } catch (...) { info.path = "[unreadable]"; }

            if (info.path.starts_with("socket:")) info.type = OpenFileType::Socket;
            else if (info.path.starts_with("pipe:")) info.type = OpenFileType::Pipe;
            else if (info.path.starts_with("anon_inode:")) info.type = OpenFileType::AnonInode;
            else {
                std::error_code ec;
                auto status = fs::status(entry.path(), ec);
                if (ec) info.type = OpenFileType::Unknown;
                else {
                    switch (status.type()) {
                        case fs::file_type::regular:
                        case fs::file_type::directory: info.type = OpenFileType::File; break;
                        case fs::file_type::character:
                        case fs::file_type::block: info.type = OpenFileType::Device; break;
                        case fs::file_type::fifo: info.type = OpenFileType::Pipe; break;
                        case fs::file_type::socket: info.type = OpenFileType::Socket; break;
                        default: info.type = OpenFileType::Other; break;
                    }
                }
            }
            openFds.push_back(info);
        }
    } catch (const fs::filesystem_error&) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return openFds;
}

utils::Result<std::vector<MemoryMapInfo>> ProcessAnalyzer::getProcessMemoryMaps(pid_t pid) const {
    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    std::vector<MemoryMapInfo> maps;
    fs::path mapsPath = procPath / std::to_string(pid) / "maps";
    auto contentOpt = utils::readTextFile(mapsPath.string());
    if (!contentOpt) return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));

    std::stringstream ss(*contentOpt);
    std::string line;
    while (std::getline(ss, line)) {
        MemoryMapInfo mapInfo;
        std::stringstream lineSs(line);
        lineSs >> std::hex >> mapInfo.startAddress;
        lineSs.ignore(1, '-');
        lineSs >> std::hex >> mapInfo.endAddress >> std::dec;
        lineSs >> mapInfo.permissions >> std::hex >> mapInfo.offset >> std::dec;
        lineSs >> mapInfo.device >> mapInfo.inode;
        std::getline(lineSs, mapInfo.pathname);
        mapInfo.pathname = utils::trim(mapInfo.pathname);
        maps.push_back(mapInfo);
    }
    return maps;
}

utils::Result<ResourceLimitInfo> ProcessAnalyzer::getProcessResourceLimits(pid_t pid) const {
    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    ResourceLimitInfo limitInfo;
    fs::path limitsPath = procPath / std::to_string(pid) / "limits";
    auto contentOpt = utils::readTextFile(limitsPath.string());
    if (!contentOpt) return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line); 

    size_t softLimitPos = line.find("Soft Limit");
    size_t hardLimitPos = line.find("Hard Limit");
    size_t unitsPos = line.find("Units");

    if (softLimitPos == std::string::npos || hardLimitPos == std::string::npos || unitsPos == std::string::npos) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    while (std::getline(ss, line)) {
        if (line.length() < unitsPos) continue;
        limitInfo.limits.push_back({
            .resource = utils::trim(line.substr(0, softLimitPos)),
            .softLimit = utils::trim(line.substr(softLimitPos, hardLimitPos - softLimitPos)),
            .hardLimit = utils::trim(line.substr(hardLimitPos, unitsPos - hardLimitPos)),
            .units = utils::trim(line.substr(unitsPos))
        });
    }
    return limitInfo;
}

utils::Result<CgroupInfo> ProcessAnalyzer::getProcessCgroupInfo(pid_t pid) const {
    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());
    
    CgroupInfo cgroupInfo;
    fs::path cgroupPath = procPath / std::to_string(pid) / "cgroup";
    auto contentOpt = utils::readTextFile(cgroupPath.string());
     if (!contentOpt) return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    
    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        CgroupEntry entry;
        std::stringstream lineSs(line);
        lineSs >> entry.id;
        lineSs.ignore(1, ':');
        std::getline(lineSs, entry.controllers, ':');
        std::getline(lineSs, entry.path);
        cgroupInfo.entries.push_back(entry);
    }
    return cgroupInfo;
}

utils::Result<void> ProcessAnalyzer::setProcessNiceness(pid_t pid, int niceness) {
    if (setpriority(PRIO_PROCESS, pid, niceness) == 0) return {};
    return std::unexpected(utils::make_error_code((errno == EACCES || errno == EPERM) ? utils::UtilsError::analyzerPermissionDenied : utils::UtilsError::analyzerSystemError));
}

utils::Result<void> ProcessAnalyzer::setProcessCpuAffinity(pid_t pid, const CpuSet& affinity) {
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int cpu : affinity.cpus) {
        CPU_SET(cpu, &set);
    }
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &set) == 0) return {};
    return std::unexpected(utils::make_error_code((errno == EACCES || errno == EPERM) ? utils::UtilsError::analyzerPermissionDenied : utils::UtilsError::analyzerSystemError));
}

