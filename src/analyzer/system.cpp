// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/core.h"

#include <filesystem>
#include <sstream>
#include <vector>
#include <charconv>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/statvfs.h>
#include <map>
#include <ranges>

namespace fs = std::filesystem;

namespace {
    template <typename TInt>
    std::optional<TInt> parseIntegerNoThrow(std::string_view text, int base = 10) {
        TInt value{};
        const char* begin = text.data();
        const char* end = begin + text.size();
        const auto [ptr, ec] = std::from_chars(begin, end, value, base);
        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }
        return value;
    }
}

utils::Result<long long> ProcessAnalyzer::getSystemBootTimeUnix(const std::filesystem::path& procPath) {
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
                    if (auto parsedPid = parseIntegerNoThrow<int>(filename)) {
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
                if (auto parsedPid = parseIntegerNoThrow<int>(filename)) {
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
                
                unsigned long long user;
                unsigned long long nice;
                unsigned long long system;
                unsigned long long idle;
                unsigned long long iowait;
                unsigned long long irq;
                unsigned long long softirq;
                unsigned long long steal;
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
        int major;
        int minor;
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
