// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ranges> // For std::ranges::copy_if
#include <pwd.h>        // For getpwuid
#include <sys/types.h>
#include <string_view> // For std::string_view::starts_with
#include <chrono>
#include <thread>

#include <unistd.h>     // For sysconf

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

    long long getTotalSystemCpuTimeTicks(std::string_view procPath) {
        std::string statPath = std::string(procPath) + "/stat";
        if (auto statContentOpt = Utils::readTextFile(statPath)) {
            std::stringstream ss(*statContentOpt);
            std::string cpuLine;
            std::getline(ss, cpuLine);
            if (cpuLine.starts_with("cpu")) {
                std::stringstream lineSs(cpuLine);
                std::string cpuLabel;
                lineSs >> cpuLabel;
                long long total = 0;
                long long time;
                while (lineSs >> time) {
                    total += time;
                }
                return total;
            }
        }
        return -1; // Indicate error
    }

    constexpr int kExamplePid1 = 100;
    constexpr int kExamplePid2 = 200;
}

ProcessAnalyzer::ProcessAnalyzer(std::string_view procPath) : procPath(procPath) {}

std::vector<int> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return {1, /* Example PID */ kExamplePid1, /* Example PID */ kExamplePid2 /* Example PID */}; 
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (Utils::isInteger(filename)) {
                    pids.push_back(std::stoi(filename));
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing /proc directory: " << e.what() << std::endl;
        // Continue with potentially incomplete PIDs or return empty if essential
    }
    return pids;
}

std::optional<ProcessInfo> ProcessAnalyzer::getProcessDetails(int pid) const {
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
    // --- Initialize new fields ---
    info.startTimeTicks = 0;
    info.executablePath = "";
    info.currentWorkingDirectory = "";
    info.cpuUserTimeTicks = 0;
    info.cpuKernelTimeTicks = 0;
    info.ioReadBytes = 0;
    info.ioWriteBytes = 0;
    info.priority = 0;

    std::string pidPath = procPath + "/" + std::to_string(pid);
    if (!fs::exists(pidPath)) {
        return std::nullopt; // Process does not exist
    }

    // Read status file for some details
    std::string statusPath = pidPath + "/status";
    if (auto statusContentOpt = Utils::readTextFile(statusPath)) {
        std::vector<std::string> lines = Utils::split(*statusContentOpt, '\n');
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
            } else if (line.starts_with("Threads:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.threadCount;
            }
        }
    } else {
        return std::nullopt; // Essential file not readable
    }

    // Read stat file for CPU times, priority, and start time
    std::string statPath = pidPath + "/stat";
    if (auto statContentOpt = Utils::readTextFile(statPath)) {
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
        for (int i = 0; i < statFieldsToSkipBeforeUtime; ++i) { std::string dummy; ss >> dummy; }
        ss >> utime >> stime >> cutime >> cstime; // 14, 15, 16, 17
        info.cpuUserTimeTicks = utime + cutime;
        info.cpuKernelTimeTicks = stime + cstime;
        // Field 18 is priority
        ss >> priority;
        info.priority = static_cast<int>(priority);
        // Skip to field 22 (starttime)
        for (int i = 0; i < statFieldsToSkipBeforeStarttime; ++i) { std::string dummy; ss >> dummy; }
        ss >> info.startTimeTicks;
    }

    // Read io file
    std::string ioPath = pidPath + "/io";
    if (auto ioContentOpt = Utils::readTextFile(ioPath)) {
        std::vector<std::string> lines = Utils::split(*ioContentOpt, '\n');
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
    if (auto cmdlineContentOpt = Utils::readTextFile(cmdlinePath)) {
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
    if (auto environContentOpt = Utils::readTextFile(environPath)) {
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
    auto pids = getPids();
    for (int pid : pids) {
        auto details = getProcessDetails(pid);
        if (details) {
            results.push_back(*details);
        }
    }
    return results;
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

std::vector<ProcessInfo> ProcessAnalyzer::getProcessesByName(std::string_view name) const {
    return findProcesses([name](const ProcessInfo& info) {
        return info.name == name;
    });
}

std::vector<ProcessInfo> ProcessAnalyzer::getProcessesByUser(std::string_view username) const {
    return findProcesses([username](const ProcessInfo& info) {
        return info.username == username;
    });
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
            if (filter.nameContains && process.name.find(*filter.nameContains) == std::string::npos) return false;
            if (filter.userFilter && process.username != *filter.userFilter) return false;
            if (filter.stateFilter && (process.state.empty() || process.state[0] != *filter.stateFilter)) return false;
            if (filter.minThreads && process.threadCount < *filter.minThreads) return false;
            if (filter.maxThreads && process.threadCount > *filter.maxThreads) return false;
            if (filter.minResidentMemoryKB && process.residentMemory < *filter.minResidentMemoryKB) return false;
            if (filter.maxResidentMemoryKB && process.residentMemory > *filter.maxResidentMemoryKB) return false;
            if (filter.minVirtualMemoryKB && process.virtualMemory < *filter.minVirtualMemoryKB) return false;
            if (filter.maxVirtualMemoryKB && process.virtualMemory > *filter.maxVirtualMemoryKB) return false;
            if (filter.cmdlineContains && process.cmdline.find(*filter.cmdlineContains) == std::string::npos) return false;
            if (filter.executablePathContains && process.executablePath.find(*filter.executablePathContains) == std::string::npos) return false;
            if (filter.uidFilter && process.uid != *filter.uidFilter) return false;
            if (filter.minPriority && process.priority < *filter.minPriority) return false;
            if (filter.maxPriority && process.priority > *filter.maxPriority) return false;
            return true;
        });

    // Apply sorting
    std::ranges::sort(filteredProcesses, 
        [&](const ProcessInfo& a, const ProcessInfo& b) {
        bool less = false;
        switch (sortBy) {
            case ProcessSortField::PID: less = a.pid < b.pid; break;
            case ProcessSortField::PPID: less = a.ppid < b.ppid; break;
            case ProcessSortField::UID: less = a.uid < b.uid; break;
            case ProcessSortField::USER: less = a.username < b.username; break;
            case ProcessSortField::NAME: less = a.name < b.name; break;
            case ProcessSortField::STATE: less = a.state < b.state; break;
            case ProcessSortField::RSS: less = a.residentMemory < b.residentMemory; break;
            case ProcessSortField::VM: less = a.virtualMemory < b.virtualMemory; break;
            case ProcessSortField::THREADS: less = a.threadCount < b.threadCount; break;
            case ProcessSortField::START_TIME: less = a.startTimeTicks < b.startTimeTicks; break;
            case ProcessSortField::EXECUTABLE_PATH: less = a.executablePath < b.executablePath; break;
            case ProcessSortField::CWD: less = a.currentWorkingDirectory < b.currentWorkingDirectory; break;
            case ProcessSortField::CPU_USER_TIME: less = a.cpuUserTimeTicks < b.cpuUserTimeTicks; break;
            case ProcessSortField::CPU_KERNEL_TIME: less = a.cpuKernelTimeTicks < b.cpuKernelTimeTicks; break;
            case ProcessSortField::IO_READ_BYTES: less = a.ioReadBytes < b.ioReadBytes; break;
            case ProcessSortField::IO_WRITE_BYTES: less = a.ioWriteBytes < b.ioWriteBytes; break;
            case ProcessSortField::PRIORITY: less = a.priority < b.priority; break;
        }

        return (sortOrder == SortOrder::ASC) ? less : !less;
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

std::vector<std::string> ProcessAnalyzer::getProcessOpenFiles(int pid) const {
    std::vector<std::string> openFiles;
    std::string fdPath = procPath + "/" + std::to_string(pid) + "/fd";

    if (!fs::exists(fdPath) || !fs::is_directory(fdPath)) {
        // Return empty if directory doesn't exist (e.g., process not found, permissions)
        return openFiles;
    }

    try {
        for (const auto& entry : fs::directory_iterator(fdPath)) {
            // Each entry in /proc/<pid>/fd is a symlink to the actual file
            openFiles.push_back(fs::read_symlink(entry.path()).string());
        }
    } catch (const fs::filesystem_error& e) {
        // Catch permission denied or other filesystem errors
        throw std::runtime_error("Failed to read open files for PID " + std::to_string(pid) + ": " + e.what());
    }
    return openFiles;
}

std::optional<ProcessCpuUsage> ProcessAnalyzer::getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const {
    auto initialDetails = getProcessDetails(pid);
    long long initialTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);

    if (!initialDetails || initialTotalSystemTicks < 0) {
        return std::nullopt;
    }

    std::this_thread::sleep_for(durationMs);

    auto finalDetails = getProcessDetails(pid);
    long long finalTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);
    
    if (!finalDetails || finalTotalSystemTicks < 0) {
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

std::vector<ProcessCpuUsage> ProcessAnalyzer::getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshot = snapshot();
    long long initialTotalSystemTicks = getTotalSystemCpuTimeTicks(procPath);

    if (initialTotalSystemTicks < 0) {
        throw std::runtime_error("Could not read initial system CPU times.");
    }
    
    std::this_thread::sleep_for(durationMs);

    auto finalSnapshot = snapshot();
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

std::vector<std::string> ProcessAnalyzer::getProcessEnvironment(int pid) const {
    std::vector<std::string> env;
    std::string environPath = procPath + "/" + std::to_string(pid) + "/environ";

    if (auto environContentOpt = Utils::readTextFile(environPath)) {
        std::string_view content = *environContentOpt;
        size_t start = 0;
        while(start < content.size()) {
            size_t end = content.find('\0', start);
            if (end == std::string_view::npos) break;
            env.emplace_back(content.substr(start, end - start));
            start = end + 1;
        }
    } else {
        // According to contract, return empty vector if inaccessible.
        // A runtime_error could be thrown for explicit permission errors if desired.
    }
    return env;
}

std::optional<ProcessInfo> ProcessAnalyzer::getParentProcess(int pid) const {
    auto details = getProcessDetails(pid);
    if (!details || details->ppid == 0) {
        return std::nullopt;
    }
    return getProcessDetails(details->ppid);
}

std::vector<ProcessInfo> ProcessAnalyzer::getAllDescendantProcesses(int pid) const {
    std::vector<ProcessInfo> descendants;
    std::vector<int> toVisit;
    toVisit.push_back(pid);

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
