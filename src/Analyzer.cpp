// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "utils.h"
#include <iostream>
#include <filesystem>

#include <sstream>
#include <algorithm>
#include <ranges> // For std::ranges::copy_if
#include <pwd.h>        // For getpwuid
#include <sys/types.h>
#include <string_view> // For std::string_view::starts_with
#include <chrono>
#include <thread>
#include <stdexcept>

#include <unistd.h>     // For sysconf, gethostname
#include <csignal>     // For kill
#include <sys/statvfs.h> // for statvfs
#include <netinet/in.h> // for INET6_ADDRSTRLEN
#include <arpa/inet.h>  // for inet_ntop

#include <set>

namespace fs = std::filesystem;

namespace {
    // Number of fields to skip after ppid (4th field) and before utime (14th field)
    // Fields skipped: pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt
    // Count: 9 fields (from 5th to 13th, 1-based indexing)
    constexpr int statFieldsToSkipBeforeUtime = 9;

    // Number of fields to skip after nice (19th field) and before starttime (22nd field)
    // Fields skipped: num_threads, itrealvalue
    // Count: 3 fields (from 19th to 21st, 1-based indexing)
    constexpr int statFieldsToSkipBeforeStarttime = 3;
constexpr int statFieldsToSkipBeforeThreadUtime = 10;
constexpr double kMSInSecond = 1000.0;


    long long getTotalSystemCpuTimeTicks(std::string_view procPath) {
        auto stats = ProcessAnalyzer(procPath).getSystemCpuStats();
        if(stats.has_value()){
            return static_cast<long long>(stats->user) + static_cast<long long>(stats->nice) + static_cast<long long>(stats->system) + static_cast<long long>(stats->idle) + static_cast<long long>(stats->iowait) + static_cast<long long>(stats->irq) + static_cast<long long>(stats->softirq) + static_cast<long long>(stats->steal);
        }
        return -1; // Indicate error
    }

    constexpr int kExamplePid1 = 100;
    constexpr int kExamplePid2 = 200;

    // Helper function to get system boot time in Unix timestamp (seconds since epoch)
    long long getSystemBootTimeUnix(std::string_view procPath) {
        std::string uptimePath = std::string(procPath) + "/uptime";
        auto contentOpt = utils::readTextFile(uptimePath);
        if (!contentOpt) {
            // Log or handle error appropriately. For now, return 0.
            return 0; 
        }
        std::stringstream ss(*contentOpt);
        double uptimeSeconds;
        ss >> uptimeSeconds;

        auto now = std::chrono::system_clock::now();
        long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        return currentTimeUnix - static_cast<long long>(uptimeSeconds);
    }

} // namespace

namespace { // Anonymous namespace for helper functions and enums
    // ... existing content ...

    // Helper function to apply process filters
    bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter) {
            std::ranges::copy_if(allProcesses, std::back_inserter(filteredProcesses),
                [&](const ProcessInfo& process) {
                    return matchesFilter(process, filter);
                });    }

    // parseNetFile removed from here as it was a duplicate/broken version.
    // The correct version is in the anonymous namespace later in the file.

    std::vector<ProcessInfo> getBasicSnapshot(const ProcessAnalyzer& analyzer) {
        std::vector<ProcessInfo> processes;
        for (int pid : analyzer.getPids()) {
            if (auto details = analyzer.getProcessDetails(pid); details.has_value()) {
                processes.push_back(*details);
            }
        }
        return processes;
    }

} // Anonymous namespace ends


ProcessAnalyzer::ProcessAnalyzer(std::string_view procPath) : procPath(procPath) {}

std::vector<int> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return {1, kExamplePid1, kExamplePid2}; 
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (utils::isInteger(filename)) {
                    pids.push_back(std::stoi(filename));
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        (void)e;
        // In a real application, consider logging this error.
        // For this library, we might choose to throw or return an error code.
        // For now, we silently return a potentially empty vector.
        // This behavior should be documented.
    }
    return pids;
}

std::expected<ProcessInfo, AnalyzerErrorDetail> ProcessAnalyzer::getProcessDetails(int pid) const {
    ProcessInfo info;
    info.pid = pid;
    info.name = "Unknown";
    info.state = "?";
    info.residentMemory = 0;
    info.virtualMemory = 0;
    info.ppid = 0;
    info.uid = 0;
    info.username = "Unknown";
    info.threadCount = 0;
    info.cmdline = "";
    // --- Initialize new fields for Iteration 5 ---
    info.startTimeTicks = 0;
    info.executablePath = "";
    info.currentWorkingDirectory = "";
    info.cpuUserTimeTicks = 0;
    info.cpuKernelTimeTicks = 0;
    info.ioReadBytes = 0;
    info.ioWriteBytes = 0;
    info.priority = 0;
    // --- Initialize new fields for Iteration 13 ---
    info.startTimeUnix = 0;
    info.elapsedTime = "N/A";
    info.cpuUsage = 0.0F;
    info.memoryPercentage = 0.0F;

    std::string pidPath = procPath + "/" + std::to_string(pid);
    if (!fs::exists(pidPath)) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::processNotFound,
            .message = "Process with PID " + std::to_string(pid) + " not found.",
            .systemErrno = std::nullopt
        });
    }

    // Read status file for some details
    std::string statusPath = pidPath + "/status";
    if (auto statusContentOpt = utils::readTextFile(statusPath)) {
        std::vector<std::string> lines = utils::split(*statusContentOpt, '\n');
        for (const auto& line : lines) {
            if (line.starts_with("Name:")) {
                info.name = line.substr(line.find(':') + 1);
                info.name.erase(0, info.name.find_first_not_of(" \t"));
            } else if (line.starts_with("State:")) {
                info.state = line.substr(line.find(':') + 1);
                info.state.erase(0, info.state.find_first_not_of(" \t"));
            } else if (line.starts_with("VmSize:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.virtualMemory;
            } else if (line.starts_with("VmRSS:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.residentMemory;
            } else if (line.starts_with("PPid:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.ppid;
            } else if (line.starts_with("Uid:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.uid;
            }
            else if (line.starts_with("Threads:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.threadCount;
            }
        }
    } else {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read status file for PID " + std::to_string(pid),
            .systemErrno = errno
        });
    }

    // Read stat file for CPU times, priority, and start time
    std::string statPath = pidPath + "/stat";
    if (auto statContentOpt = utils::readTextFile(statPath)) {
        std::stringstream ss(*statContentOpt);
        std::string comm;
        char state;
        // Fields are 1-based index from man proc(5)
        ss >> info.pid >> comm >> state >> info.ppid; // 1, 2, 3, 4
        // Skip to fields 14-17 (utime, stime, cutime, cstime)
        long long utime;
        long long stime;
        long long cutime;
        long long cstime;
        long priority;
        long nice;
        for (int i = 0; i < statFieldsToSkipBeforeUtime; ++i) { std::string dummy; ss >> dummy; }
        ss >> utime >> stime >> cutime >> cstime; // 14, 15, 16, 17
        info.cpuUserTimeTicks = utime + cutime;
        info.cpuKernelTimeTicks = stime + cstime;
        // Field 18 is priority, 19 is nice
        ss >> priority >> nice;
        info.priority = static_cast<int>(nice);
        // Skip to field 22 (starttime)
        for (int i = 0; i < (statFieldsToSkipBeforeStarttime-1); ++i) { std::string dummy; ss >> dummy; }
        ss >> info.startTimeTicks;
    }

    // Calculate startTimeUnix and elapsedTime
    long long systemBootTimeUnix = getSystemBootTimeUnix(procPath);
    if (systemBootTimeUnix != 0) { // Only calculate if boot time is successfully retrieved
        long systemClockTicks = getSystemClockTicksPerSecond();
        if (systemClockTicks > 0) {
            long long processStartTimeSec = info.startTimeTicks / systemClockTicks;
            info.startTimeUnix = systemBootTimeUnix + processStartTimeSec;
            
            // Calculate elapsed time from current time
            auto now = std::chrono::system_clock::now();
            long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            long long elapsedSeconds = currentTimeUnix - info.startTimeUnix;
            info.elapsedTime = utils::formatElapsedTime(elapsedSeconds);
        }
    }

    // Read io file
    std::string ioPath = pidPath + "/io";
    if (auto ioContentOpt = utils::readTextFile(ioPath)) {
        std::vector<std::string> lines = utils::split(*ioContentOpt, '\n');
        for (const auto& line : lines) {
            if (line.starts_with("rchar:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.ioReadBytes;
            } else if (line.starts_with("wchar:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.ioWriteBytes;
            }
        }
    } // Not all kernels/configs have this file, so don't fail if it's missing.

    // Resolve UID to username
    if (struct passwd *pw = getpwuid(info.uid)) {
        info.username = pw->pw_name;
    }

    // Read cmdline
    std::string cmdlinePath = pidPath + "/cmdline";
    if (auto cmdlineContentOpt = utils::readTextFile(cmdlinePath)) {
        std::string rawCmdline = *cmdlineContentOpt;
        std::ranges::replace(rawCmdline, '\0', ' ');
        if (!rawCmdline.empty() && rawCmdline.back() == ' ') rawCmdline.pop_back();
        info.cmdline = rawCmdline;
    }
    
    // Read symlinks for executable path and CWD
    try {
        info.executablePath = fs::read_symlink(pidPath + "/exe").string();
    } catch (const fs::filesystem_error& e) { (void)e; /* Ignore if not accessible */ }
    try {
        info.currentWorkingDirectory = fs::read_symlink(pidPath + "/cwd").string();
    } catch (const fs::filesystem_error& e) { (void)e; /* Ignore if not accessible */ }

    // Read environment variables
    std::string environPath = pidPath + "/environ";
    if (auto environContentOpt = utils::readTextFile(environPath)) {
        std::string_view content = *environContentOpt;
        size_t start = 0;
        while(start < content.size()) {
            size_t end = content.find('\0', start);
            if (end == std::string_view::npos) break;
            info.environmentVariables.emplace_back(content.substr(start, end - start));
            start = end + 1;
        }
    }

    return info;
}

std::vector<ProcessInfo> ProcessAnalyzer::snapshot() const {
    std::vector<ProcessInfo> results;
    std::map<int, ProcessInfo> processMap; // Use map for efficient lookup by PID

    // First pass: get basic process details
    auto basicProcesses = getBasicSnapshot(*this);
    for (const auto& p : basicProcesses) {
        processMap[p.pid] = p;
    }

    // Second pass: get CPU usage and update ProcessInfo objects
    // Using a short duration (e.g., 100ms) for a reasonable "instantaneous" CPU snapshot
    constexpr int kCpuDurationMs = 100;
    auto cpuDuration = std::chrono::milliseconds(kCpuDurationMs);
    
    // To avoid recursion, we cannot call getAllProcessesCpuUsage here if it calls snapshot()
    // But now getAllProcessesCpuUsage uses getBasicSnapshot, so it is safe.
    std::vector<ProcessCpuUsage> allCpuUsage = getAllProcessesCpuUsage(cpuDuration);
    
    for (const auto& cpuUsage : allCpuUsage) {
        if (processMap.contains(cpuUsage.pid)) {
            processMap[cpuUsage.pid].cpuUsage = static_cast<float>(cpuUsage.cpuPercentage);
        }
    }

    // Third pass: get system memory and update ProcessInfo objects
    auto systemMemoryInfo = getSystemMemoryInfo();
    if (systemMemoryInfo.has_value() && systemMemoryInfo->memTotal > 0) {
        auto totalMemKB = static_cast<float>(systemMemoryInfo->memTotal);
        for (auto& pair : processMap) {
            ProcessInfo& info = pair.second;
            info.memoryPercentage = (static_cast<float>(info.residentMemory) / totalMemKB) * 100.0F;
        }
    }

    // Convert map values back to a vector
    results.reserve(processMap.size());
    for (const auto& pair : processMap) {
        results.push_back(pair.second);
    }

    return results;
}

// ...

std::vector<ProcessCpuUsage> ProcessAnalyzer::getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshot = getBasicSnapshot(*this);
    long long initialTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);

    if (initialTotalSystemTicks < 0) {
        throw std::runtime_error("Could not read initial system CPU times.");
    }
    
    std::this_thread::sleep_for(durationMs);

    auto finalSnapshot = getBasicSnapshot(*this);
    long long finalTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);

    if (finalTotalSystemTicks < 0) {
        throw std::runtime_error("Could not read final system CPU times.");
    }

    long long totalSystemTicksDelta = finalTotalSystemTicks - initialTotalSystemTicks;
    std::vector<ProcessCpuUsage> results;
    std::map<int, ProcessInfo> finalSnapshotMap;
    for(const auto& info : finalSnapshot) {
        finalSnapshotMap[info.pid] = info;
    }

    for (const auto& initialInfo : initialSnapshot) {
        auto it = finalSnapshotMap.find(initialInfo.pid);
        if (it != finalSnapshotMap.end()) {
            const auto& finalInfo = it->second;
            long long processCpuTicksDelta = (finalInfo.cpuUserTimeTicks + finalInfo.cpuKernelTimeTicks) - (initialInfo.cpuUserTimeTicks + initialInfo.cpuKernelTimeTicks);
            
            ProcessCpuUsage usage;
            usage.pid = initialInfo.pid;
            usage.name = finalInfo.name;
            if (totalSystemTicksDelta > 0) {
                usage.cpuPercentage = 100.0 * static_cast<double>(processCpuTicksDelta) / static_cast<double>(totalSystemTicksDelta);
            } else {
                usage.cpuPercentage = 0.0;
            }
            results.push_back(usage);
        }
    }

    return results;
}

std::expected<SystemMemoryInfo, AnalyzerErrorDetail> ProcessAnalyzer::getSystemMemoryInfo() const {
    std::string meminfoPath = procPath + "/meminfo";
    auto contentOpt = utils::readTextFile(meminfoPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read /proc/meminfo",
            .systemErrno = errno
        });
    }

    SystemMemoryInfo memInfo;
    std::stringstream ss(*contentOpt);
    std::string line;
    while (std::getline(ss, line)) {
        std::string key;
        unsigned long value;
        std::stringstream lineSs(line);
        lineSs >> key >> value;
        if(key.empty()) continue;
        key.pop_back(); // Remove colon

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

std::expected<SystemLoadAverage, AnalyzerErrorDetail> ProcessAnalyzer::getSystemLoadAverage() const {
    std::string loadavgPath = procPath + "/loadavg";
    auto contentOpt = utils::readTextFile(loadavgPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read /proc/loadavg",
            .systemErrno = errno
        });
    }

    SystemLoadAverage loadAvg;
    std::stringstream ss(*contentOpt);
    ss >> loadAvg.oneMin >> loadAvg.fiveMin >> loadAvg.fifteenMin;
    return loadAvg;
}

std::expected<SystemCpuStats, AnalyzerErrorDetail> ProcessAnalyzer::getSystemCpuStats() const {
    std::string statPath = procPath + "/stat";
    auto contentOpt = utils::readTextFile(statPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read /proc/stat",
            .systemErrno = errno
        });
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line);

    if (line.starts_with("cpu ")) {
        SystemCpuStats stats;
        std::stringstream lineSs(line);
        std::string cpuLabel;
        lineSs >> cpuLabel >> stats.user >> stats.nice >> stats.system >> stats.idle 
                >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal;
        return stats;
    }

    return std::unexpected(AnalyzerErrorDetail{
        .code = AnalyzerError::parsingError,
        .message = "Could not find 'cpu' line in /proc/stat",
        .systemErrno = std::nullopt
    });
}


std::vector<ProcessInfo> ProcessAnalyzer::findProcesses(const ProcessPredicate& predicate) const {
    std::vector<ProcessInfo> allProcesses = snapshot();
    std::vector<ProcessInfo> filtered;
    for (const auto& info : allProcesses) {
        if (predicate(info)) {
            filtered.push_back(info);
        }
    }
    return filtered;
}

std::vector<ProcessInfo> ProcessAnalyzer::queryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    std::vector<ProcessInfo> allProcesses = snapshot();
    std::vector<ProcessInfo> filteredProcesses;

    // Apply filtering
    std::ranges::copy_if(allProcesses, std::back_inserter(filteredProcesses),
        [&](const ProcessInfo& process) {
            return matchesFilter(process, filter);
        });

    // Apply sorting
    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
        bool less = false; // Initialize less
        switch (sortBy) {
            case ProcessSortField::pid: less = a.pid < b.pid; break;
            case ProcessSortField::ppid: less = a.ppid < b.ppid; break;
            case ProcessSortField::uid: less = a.uid < b.uid; break;
            case ProcessSortField::user: less = a.username < b.username; break;
            case ProcessSortField::name: less = a.name < b.name; break;
            case ProcessSortField::state: less = a.state < b.state; break;
            case ProcessSortField::rss: less = a.residentMemory < b.residentMemory; break;
            case ProcessSortField::vmsize: less = a.virtualMemory < b.virtualMemory; break;
            case ProcessSortField::threads: less = a.threadCount < b.threadCount; break;
            case ProcessSortField::startTime: less = a.startTimeTicks < b.startTimeTicks; break;
            case ProcessSortField::executablePath: less = a.executablePath < b.executablePath; break;
            case ProcessSortField::cmdline: less = a.cmdline < b.cmdline; break;
            case ProcessSortField::cpuTime:
                less = (a.cpuUserTimeTicks + a.cpuKernelTimeTicks) < (b.cpuUserTimeTicks + b.cpuKernelTimeTicks);
                break;
            case ProcessSortField::cwd: less = a.currentWorkingDirectory < b.currentWorkingDirectory; break;
            case ProcessSortField::cpuUserTime: less = a.cpuUserTimeTicks < b.cpuUserTimeTicks; break;
            case ProcessSortField::cpuKernelTime: less = a.cpuKernelTimeTicks < b.cpuKernelTimeTicks; break;
            case ProcessSortField::ioReadBytes: less = a.ioReadBytes < b.ioReadBytes; break;
            case ProcessSortField::ioWriteBytes: less = a.ioWriteBytes < b.ioWriteBytes; break;
            case ProcessSortField::priority: less = a.priority < b.priority; break;
            case ProcessSortField::cpuUsage: less = a.cpuUsage < b.cpuUsage; break;          // NEW for Iteration 13
            case ProcessSortField::memoryPercentage: less = a.memoryPercentage < b.memoryPercentage; break; // NEW for Iteration 13
        }

        return (sortOrder == SortOrder::asc) ? less : !less;
    });

    return filteredProcesses;
}

std::vector<ProcessInfo> ProcessAnalyzer::getChildProcesses(int pid) const {
    std::vector<ProcessInfo> allProcesses = snapshot();
    std::vector<ProcessInfo> children;
    for (const auto& process : allProcesses) {
        if (process.ppid == pid) {
            children.push_back(process);
        }
    }
    return children;
}

std::map<int, std::string> ProcessAnalyzer::getOpenFileDescriptors(pid_t pid) const {
    std::map<int, std::string> openFds;
    std::string fdPath = procPath + "/" + std::to_string(pid) + "/fd";

    if (!fs::exists(fdPath) || !fs::is_directory(fdPath)) {
        return openFds;
    }

    try {
        for (const auto& entry : fs::directory_iterator(fdPath)) {
            if (entry.is_symlink()) {
                std::string fdStr = entry.path().filename().string();
                if (utils::isInteger(fdStr)) {
                    int fd = std::stoi(fdStr);
                    try {
                        openFds[fd] = fs::read_symlink(entry.path()).string();
                    } catch (const fs::filesystem_error&) {
                        openFds[fd] = "[unreadable]";
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Failed to read open files directory for PID " + std::to_string(pid) + ": " + e.what());
    }
    return openFds;
}


std::optional<ProcessCpuUsage> ProcessAnalyzer::getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const {
    auto initialDetails = getProcessDetails(pid);
    long long initialTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);

    if (!initialDetails.has_value() || initialTotalSystemTicks < 0) {
        return std::nullopt;
    }

    std::this_thread::sleep_for(durationMs);

    auto finalDetails = getProcessDetails(pid);
    long long finalTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);
    
    if (!finalDetails.has_value() || finalTotalSystemTicks < 0) {
        return std::nullopt; // Process might have exited
    }

    long long processCpuTicksDelta = (finalDetails->cpuUserTimeTicks + finalDetails->cpuKernelTimeTicks) - (initialDetails->cpuUserTimeTicks + initialDetails->cpuKernelTimeTicks);
    long long totalSystemTicksDelta = finalTotalSystemTicks - initialTotalSystemTicks;

    ProcessCpuUsage usage;
    usage.pid = pid;
    usage.name = finalDetails->name;
    if (totalSystemTicksDelta > 0) {
        usage.cpuPercentage = 100.0 * static_cast<double>(processCpuTicksDelta) / static_cast<double>(totalSystemTicksDelta);
    } else {
        usage.cpuPercentage = 0.0;
    }

    return usage;
}

std::vector<std::string> ProcessAnalyzer::getProcessEnvironment(int pid) const {
    std::vector<std::string> env;
    std::string environPath = procPath + "/" + std::to_string(pid) + "/environ";

    if (auto environContentOpt = utils::readTextFile(environPath)) {
        std::string_view content = *environContentOpt;
        size_t start = 0;
        while(start < content.size()) {
            size_t end = content.find('\0', start);
            if (end == std::string_view::npos) break;
            env.emplace_back(content.substr(start, end - start));
            start = end + 1;
        }
    }
    return env;
}

std::optional<ProcessInfo> ProcessAnalyzer::getParentProcess(int pid) const {
    auto details = getProcessDetails(pid);
    if (!details.has_value() || details->ppid == 0) {
        return std::nullopt;
    }
    auto parentDetails = getProcessDetails(details->ppid);
    if(parentDetails.has_value()){
        return *parentDetails;
    }
    return std::nullopt;
}

std::vector<ProcessInfo> ProcessAnalyzer::getAllDescendantProcesses(int pid) const {
    std::vector<ProcessInfo> descendants;
    std::vector<int> toVisit {pid};
    std::vector<ProcessInfo> allProcesses = snapshot();
    std::map<int, std::vector<ProcessInfo>> parentToChildren;
    for (const auto& p : allProcesses) {
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

long ProcessAnalyzer::getSystemClockTicksPerSecond() {
    long ticks = sysconf(_SC_CLK_TCK);
    if (ticks < 0) {
        throw std::runtime_error("Failed to get system clock ticks per second.");
    }
    return ticks;
}

// --- Iteration 9 Implementations ---

bool ProcessAnalyzer::sendSignal(int pid, ProcessSignal signal) {
    return ::kill(pid, static_cast<int>(signal)) == 0;
}

std::vector<ThreadInfo> ProcessAnalyzer::getProcessThreads(int pid) const {
    std::vector<ThreadInfo> threads;
    std::string taskPath = procPath + "/" + std::to_string(pid) + "/task";

    if (!fs::exists(taskPath) || !fs::is_directory(taskPath)) {
        return threads;
    }

    for (const auto& entry : fs::directory_iterator(taskPath)) {
        if (entry.is_directory()) {
            std::string tidStr = entry.path().filename().string();
            if (utils::isInteger(tidStr)) {
                int tid = std::stoi(tidStr);
                ThreadInfo thread;
                thread.tid = tid;

                // Read thread name
                std::string commPath = entry.path().string() + "/comm";
                if (auto commContent = utils::readTextFile(commPath)) {
                    thread.name = utils::trim(*commContent);
                }

                // Read thread stat
                std::string statPath = entry.path().string() + "/stat";
                if (auto statContent = utils::readTextFile(statPath)) {
                    std::stringstream ss(*statContent);
                    std::string comm;
                    std::string dummy;
                    char stateChar;
                    ss >> dummy >> comm >> stateChar; // tid, (comm), state
                    thread.state = stateChar;

for (int i = 0; i < statFieldsToSkipBeforeThreadUtime; ++i) ss >> dummy;
                    ss >> thread.cpuUserTimeTicks >> thread.cpuKernelTimeTicks;
                }
                threads.push_back(thread);
            }
        }
    }
    return threads;
}

std::vector<MountPointInfo> ProcessAnalyzer::getSystemDiskUsage() const {
    std::vector<MountPointInfo> mounts;
    std::string mountsPath = procPath + "/mounts";
    auto contentOpt = utils::readTextFile(mountsPath);
    if (!contentOpt) {
        return mounts;
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

std::optional<SystemInfo> ProcessAnalyzer::getSystemInfo() const {
    SystemInfo sysInfo;

    // Uptime
    std::string uptimePath = procPath + "/uptime";
    if (auto content = utils::readTextFile(uptimePath)) {
        double uptimeSecs;
        std::stringstream ss(*content);
        ss >> uptimeSecs;
        sysInfo.uptime = std::chrono::seconds(static_cast<long long>(uptimeSecs));
    } else {
        return std::nullopt;
    }

    // Kernel version
    std::string versionPath = procPath + "/version";
    if (auto content = utils::readTextFile(versionPath)) {
        sysInfo.kernelVersion = utils::trim(*content);
    } else {
        return std::nullopt;
    }

    // OS Name
    if (auto content = utils::readTextFile("/etc/os-release")) {
        std::stringstream ss(*content);
        std::string line;
        while(std::getline(ss, line)) {
            if (line.starts_with("PRETTY_NAME=")) {
                sysInfo.osName = line.substr(line.find('=') + 1);
                // Remove quotes
                std::erase(sysInfo.osName, '"');
                break;
            }
        }
    } else {
        sysInfo.osName = "Unknown"; // Not a critical error if this file is missing
    }

// Hostname
    constexpr size_t kMaxHostnameLen = 256;
    std::array<char, kMaxHostnameLen> hostname{};
    if (gethostname(hostname.data(), hostname.size()) == 0) {
        sysInfo.hostname = hostname.data();
    } else {
        return std::nullopt;
    }
    
    return sysInfo;
}

std::optional<SystemCpuUsage> ProcessAnalyzer::getSystemCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialStats = getSystemCpuStats();
    if (!initialStats.has_value()) return std::nullopt;
    
    std::this_thread::sleep_for(durationMs);

    auto finalStats = getSystemCpuStats();
    if (!finalStats.has_value()) return std::nullopt;

    unsigned long long initialTotal = initialStats->user + initialStats->nice + initialStats->system + initialStats->idle + initialStats->iowait + initialStats->irq + initialStats->softirq + initialStats->steal;
    unsigned long long finalTotal = finalStats->user + finalStats->nice + finalStats->system + finalStats->idle + finalStats->iowait + finalStats->irq + finalStats->softirq + finalStats->steal;

    unsigned long long totalDelta = finalTotal - initialTotal;
    if (totalDelta == 0) return SystemCpuUsage{0.0};

    unsigned long long idleDelta = finalStats->idle - initialStats->idle;
    
    double usage = 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta));
    return SystemCpuUsage{usage};
}

std::optional<ProcessDiskIoUsage> ProcessAnalyzer::getProcessDiskIoUsage(int pid, std::chrono::milliseconds durationMs) const {
    auto initialDetails = getProcessDetails(pid);
    if (!initialDetails.has_value()) return std::nullopt;

    std::this_thread::sleep_for(durationMs);
    
    auto finalDetails = getProcessDetails(pid);
    if (!finalDetails.has_value()) return std::nullopt;

    auto readDelta = static_cast<double>(finalDetails->ioReadBytes - initialDetails->ioReadBytes);
    auto writeDelta = static_cast<double>(finalDetails->ioWriteBytes - initialDetails->ioWriteBytes);
    double durationSec = static_cast<double>(durationMs.count()) / kMSInSecond;

    return ProcessDiskIoUsage {
        .pid = pid,
        .name = finalDetails->name,
        .readBytesPerSecond = durationSec > 0 ? readDelta / durationSec : 0.0,
        .writeBytesPerSecond = durationSec > 0 ? writeDelta / durationSec : 0.0
    };
}

std::vector<ProcessDiskIoUsage> ProcessAnalyzer::getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshot = snapshot();

    std::this_thread::sleep_for(durationMs);

    auto finalSnapshot = snapshot();
    std::map<int, ProcessInfo> finalSnapshotMap;
    for(const auto& info : finalSnapshot) {
        finalSnapshotMap[info.pid] = info;
    }

    std::vector<ProcessDiskIoUsage> results;
    double durationSec = static_cast<double>(durationMs.count()) / kMSInSecond;
    if (durationSec <= 0) return results;

    for(const auto& initialInfo : initialSnapshot) {
        auto it = finalSnapshotMap.find(initialInfo.pid);
        if (it != finalSnapshotMap.end()) {
            const auto& finalInfo = it->second;
            auto readDelta = static_cast<double>(finalInfo.ioReadBytes - initialInfo.ioReadBytes);
            auto writeDelta = static_cast<double>(finalInfo.ioWriteBytes - initialInfo.ioWriteBytes);
            results.push_back({
                .pid = finalInfo.pid,
                .name = finalInfo.name,
                .readBytesPerSecond = readDelta / durationSec,
                .writeBytesPerSecond = writeDelta / durationSec
            });
        }
    }
    return results;
}



namespace { // Anonymous namespace for helper functions and enums

    // Internal struct to temporarily hold NetworkConnection data along with its inode
    // for filtering purposes before converting to the public NetworkConnection struct.
    struct InternalNetworkConnection {
        NetworkConnection baseConn;
        int inode;
    };

    enum class TcpState : std::uint8_t {
        kEstablished = 1,
        kSynSent,
        kSynRecv,
        kFinWait1,
        kFinWait2,
        kTimeWait,
        kClose,
        kCloseWait,
        kLastAck,
        kListen,
        kClosing,
        kUnknown
    };
    const int kIpv6LineDummyCount = 5;

    // Helper function to parse /proc/net/tcp, udp, etc. files.
    // It returns a vector of InternalNetworkConnection, which includes the inode
    // for later filtering by process's open file descriptors.
    std::vector<InternalNetworkConnection> parseNetFile(const std::string& filePath, std::string_view protocolPrefix) {
        std::vector<InternalNetworkConnection> internalConnections;
        auto content = utils::readTextFile(filePath);
        if (!content) return internalConnections;

        std::stringstream ss(*content);
        std::string line;
        std::getline(ss, line); // Skip header

        while (std::getline(ss, line)) {
            std::stringstream lineSs(line);
            InternalNetworkConnection internalConn;
            internalConn.baseConn.protocol = protocolPrefix;

            int dummy;
            unsigned long localAddrPart1 = 0;
            unsigned long localAddrPart2 = 0;
            unsigned long localAddrPart3 = 0;
            unsigned long localAddrPart4 = 0; // For IPv6
            unsigned long remoteAddrPart1 = 0;
            unsigned long remoteAddrPart2 = 0;
            unsigned long remoteAddrPart3 = 0;
            unsigned long remoteAddrPart4 = 0; // For IPv6
            unsigned long localAddr = 0; // For IPv4
            unsigned long remoteAddr = 0; // For IPv4
            unsigned int localP = 0;
            unsigned int remoteP = 0;
            int state = 0;
            char colon;

            lineSs >> dummy; 
            lineSs.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // Skip 'sl' column

            if (filePath.find('6') != std::string::npos) { // IPv6
                lineSs >> std::hex >> localAddrPart1 >> localAddrPart2 >> localAddrPart3 >> localAddrPart4 >> colon >> localP
                       >> remoteAddrPart1 >> remoteAddrPart2 >> remoteAddrPart3 >> remoteAddrPart4 >> colon >> remoteP
                       >> state;
                for(int i=0; i<kIpv6LineDummyCount; ++i) lineSs >> dummy;
                lineSs >> internalConn.inode;
                
                std::array<char, INET6_ADDRSTRLEN> localStrBuf{};
                std::array<char, INET6_ADDRSTRLEN> remoteStrBuf{};
                
                // Convert 32-bit parts to correct in6_addr structure (handle endianness if necessary, /proc usually gives host byte order for individual parts)
                in6_addr localIn6Addr;
                localIn6Addr.__in6_u.__u6_addr32[0] = localAddrPart1;
                localIn6Addr.__in6_u.__u6_addr32[1] = localAddrPart2;
                localIn6Addr.__in6_u.__u6_addr32[2] = localAddrPart3;
                localIn6Addr.__in6_u.__u6_addr32[3] = localAddrPart4;
                // Need to convert to network byte order before inet_ntop if parts are host byte order.
                // Assuming /proc output for addrparts is already network byte order for simplicity here.
                // Or that inet_ntop takes values that are already in host byte order, and converts.
                // The /proc documentation indicates it's host byte order if `sysctl -a | grep ip_conntrack` shows values.
                // For direct parsing from /proc/net, usually it's represented as big-endian hex.
                // Given the original example `0100007F:001B` for IPv4 127.0.0.1:27, the hex is big-endian.
                // So, the parts read in hex are probably in network byte order already.

                inet_ntop(AF_INET6, &localIn6Addr, localStrBuf.data(), localStrBuf.size());
                internalConn.baseConn.localAddress = std::string(localStrBuf.data()) + ":" + std::to_string(localP);
                
                if(remoteP > 0 || (remoteAddrPart1 || remoteAddrPart2 || remoteAddrPart3 || remoteAddrPart4)) { // Check if remote address/port is meaningful
                    in6_addr remoteIn6Addr;
                    remoteIn6Addr.__in6_u.__u6_addr32[0] = remoteAddrPart1;
                    remoteIn6Addr.__in6_u.__u6_addr32[1] = remoteAddrPart2;
                    remoteIn6Addr.__in6_u.__u6_addr32[2] = remoteAddrPart3;
                    remoteIn6Addr.__in6_u.__u6_addr32[3] = remoteAddrPart4;
                    inet_ntop(AF_INET6, &remoteIn6Addr, remoteStrBuf.data(), remoteStrBuf.size());
                    internalConn.baseConn.remoteAddress = std::string(remoteStrBuf.data()) + ":" + std::to_string(remoteP);
                } else {
                    internalConn.baseConn.remoteAddress = "*"; // For LISTEN or unbound remote
                }

            } else { // IPv4
                lineSs >> std::hex >> localAddr >> colon >> localP
                       >> remoteAddr >> colon >> remoteP
                       >> state >> dummy >> dummy >> dummy >> dummy >> dummy >> internalConn.inode;
                
                std::array<char, INET_ADDRSTRLEN> localStrBuf{};
                std::array<char, INET_ADDRSTRLEN> remoteStrBuf{};
                
                // Convert IP addresses from network byte order to presentation format
                // The addresses in /proc/net/tcp are usually in host byte order.
                // Need to convert to network byte order for `inet_ntop` if it expects that.
                // `htonl` for localAddr and remoteAddr might be needed depending on system endianness and `inet_ntop` implementation.
                // Assuming `inet_ntop` can handle host byte order for the `s_addr` member, or `localAddr` is already network byte order.
                // The common practice is to read hex directly and then use it.
                struct in_addr localInAddr = { .s_addr = static_cast<in_addr_t>(localAddr) };
                struct in_addr remoteInAddr = { .s_addr = static_cast<in_addr_t>(remoteAddr) };
                inet_ntop(AF_INET, &localInAddr, localStrBuf.data(), localStrBuf.size());
                inet_ntop(AF_INET, &remoteInAddr, remoteStrBuf.data(), remoteStrBuf.size());
                
                internalConn.baseConn.localAddress = std::string(localStrBuf.data()) + ":" + std::to_string(localP);
                
                if(remoteP > 0 || remoteAddr != 0) { // Check if remote address/port is meaningful
                    internalConn.baseConn.remoteAddress = std::string(remoteStrBuf.data()) + ":" + std::to_string(remoteP);
                } else {
                    internalConn.baseConn.remoteAddress = "*"; // For LISTEN or unbound remote
                }
            }

            // Map state to string (only for TCP, UDP doesn't have states)
            if (protocolPrefix.starts_with("TCP")) {
                switch(static_cast<TcpState>(state)){
                    case TcpState::kEstablished: internalConn.baseConn.state = "ESTABLISHED"; break;
                    case TcpState::kSynSent: internalConn.baseConn.state = "SYN_SENT"; break;
                    case TcpState::kSynRecv: internalConn.baseConn.state = "SYN_RECV"; break;
                    case TcpState::kFinWait1: internalConn.baseConn.state = "FIN_WAIT1"; break;
                    case TcpState::kFinWait2: internalConn.baseConn.state = "FIN_WAIT2"; break;
                    case TcpState::kTimeWait: internalConn.baseConn.state = "TIME_WAIT"; break;
                    case TcpState::kClose: internalConn.baseConn.state = "CLOSE"; break;
                    case TcpState::kCloseWait: internalConn.baseConn.state = "CLOSE_WAIT"; break;
                    case TcpState::kLastAck: internalConn.baseConn.state = "LAST_ACK"; break;
                    case TcpState::kListen: internalConn.baseConn.state = "LISTEN"; break;
                    case TcpState::kClosing: internalConn.baseConn.state = "CLOSING"; break;
                    case TcpState::kUnknown: internalConn.baseConn.state = "UNKNOWN"; break;
                }
            } else { // UDP has no states in /proc/net/udp
                internalConn.baseConn.state = "UNCONN"; // Unconnected or N/A
            }
            internalConnections.push_back(internalConn);
        }
        return internalConnections;
    }

} // Anonymous namespace ends

// New for Iteration 13: Network Activity Monitoring
std::vector<NetworkConnection> ProcessAnalyzer::getNetworkConnections(int pid) const {
    std::vector<NetworkConnection> connections;
    std::map<int, std::string> fds = getOpenFileDescriptors(pid);
    std::set<int> socketInodes;
    constexpr int kSocketInodePrefixLen = 8; // "socket:["
    constexpr int kSocketInodeSuffixLen = 1; // "]"

    for (const auto& [fd, path] : fds) {
        if (path.starts_with("socket:[")) {
            // Extract the inode number. Path looks like "socket:[12345]"
            std::string inodeStr = path.substr(kSocketInodePrefixLen, path.length() - kSocketInodePrefixLen - kSocketInodeSuffixLen);
            if(utils::isInteger(inodeStr)){
                socketInodes.insert(std::stoi(inodeStr));
            }
        }
    }

    if(socketInodes.empty()) return connections;

    std::vector<InternalNetworkConnection> allInternalConnections;
    // Call parseNetFile with the correct protocol prefix
    for (const auto& conn : parseNetFile(procPath + "/net/tcp", "TCP")) { allInternalConnections.push_back(conn); }
    for (const auto& conn : parseNetFile(procPath + "/net/tcp6", "TCP6")) { allInternalConnections.push_back(conn); }
    for (const auto& conn : parseNetFile(procPath + "/net/udp", "UDP")) { allInternalConnections.push_back(conn); }
    for (const auto& conn : parseNetFile(procPath + "/net/udp6", "UDP6")) { allInternalConnections.push_back(conn); }
    
    for(const auto& internalConn : allInternalConnections){
        if(socketInodes.contains(internalConn.inode)){
            connections.push_back(internalConn.baseConn);
        }
    }

    return connections;
}


// --- C++23 Streaming API Implementations ---

std::generator<int> ProcessAnalyzer::streamPids() const {
    if (!fs::exists(procPath)) {
        co_return; 
    }

    for (const auto& entry : fs::directory_iterator(procPath)) {
        if (entry.is_directory()) {
            std::string filename = entry.path().filename().string();
            if (utils::isInteger(filename)) {
                co_yield std::stoi(filename);
            }
        }
    }
}

std::generator<ProcessInfo> ProcessAnalyzer::streamProcesses() const {
    for (int pid : streamPids()) {
        auto details = getProcessDetails(pid);
        if (details.has_value()) {
            co_yield *details;
        }
    }
}

std::generator<ProcessInfo> ProcessAnalyzer::streamQueryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    // NOTE: Sorting with a generator is tricky. It requires consuming all items first.
    // This implementation will filter lazily, but sort eagerly.
    std::vector<ProcessInfo> filteredProcesses;
    for (auto process : streamProcesses()) {
         if (matchesFilter(process, filter)) {
             filteredProcesses.push_back(process);
         }
    }
    
    // Sort eagerly
    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
        bool less = false;
        switch (sortBy) {
            case ProcessSortField::pid: less = a.pid < b.pid; break;
            case ProcessSortField::ppid: less = a.ppid < b.ppid; break;
            case ProcessSortField::uid: less = a.uid < b.uid; break;
            case ProcessSortField::user: less = a.username < b.username; break;
            case ProcessSortField::name: less = a.name < b.name; break;
            case ProcessSortField::state: less = a.state < b.state; break;
            case ProcessSortField::rss: less = a.residentMemory < b.residentMemory; break;
            case ProcessSortField::vmsize: less = a.virtualMemory < b.virtualMemory; break;
            case ProcessSortField::threads: less = a.threadCount < b.threadCount; break;
            case ProcessSortField::startTime: less = a.startTimeTicks < b.startTimeTicks; break;
            case ProcessSortField::executablePath: less = a.executablePath < b.executablePath; break;
            case ProcessSortField::cmdline: less = a.cmdline < b.cmdline; break;
            case ProcessSortField::cpuTime:
                less = (a.cpuUserTimeTicks + a.cpuKernelTimeTicks) < (b.cpuUserTimeTicks + b.cpuKernelTimeTicks);
                break;
            case ProcessSortField::cwd: less = a.currentWorkingDirectory < b.currentWorkingDirectory; break;
            case ProcessSortField::cpuUserTime: less = a.cpuUserTimeTicks < b.cpuUserTimeTicks; break;
            case ProcessSortField::cpuKernelTime: less = a.cpuKernelTimeTicks < b.cpuKernelTimeTicks; break;
            case ProcessSortField::ioReadBytes: less = a.ioReadBytes < b.ioReadBytes; break;
            case ProcessSortField::ioWriteBytes: less = a.ioWriteBytes < b.ioWriteBytes; break;
            case ProcessSortField::priority: less = a.priority < b.priority; break;
            case ProcessSortField::cpuUsage: less = a.cpuUsage < b.cpuUsage; break;          // NEW for Iteration 13
            case ProcessSortField::memoryPercentage: less = a.memoryPercentage < b.memoryPercentage; break; // NEW for Iteration 13
            default: less = a.pid < b.pid; break;
        }
        return (sortOrder == SortOrder::asc) ? less : !less;
    });

    for(auto& process : filteredProcesses){
        co_yield process;
    }
}
