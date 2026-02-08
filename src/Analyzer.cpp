// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <pwd.h>        // For getpwuid
#include <sys/types.h>
#include <string_view> // For std::string_view::starts_with

namespace fs = std::filesystem;

namespace {
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

    std::string pidPath = procPath + "/" + std::to_string(pid);
    if (!fs::exists(pidPath)) {
        return std::nullopt; // Process does not exist
    }

    // Read status file
    std::string statusPath = pidPath + "/status";
    auto statusContentOpt = Utils::readTextFile(statusPath);
    if (!statusContentOpt) {
        return std::nullopt; // Could not read status file
    }
    const std::string& statusContent = *statusContentOpt;
    
    std::vector<std::string> lines = Utils::split(statusContent, '\n');
    for (const auto& line : lines) {
        if (std::string_view(line).starts_with("Name:")) {
            info.name = line.substr(line.find(':') + 1);
            size_t first = info.name.find_first_not_of(" \t");
            if (first != std::string::npos) info.name = info.name.substr(first);
            else info.name = "";
        } else if (std::string_view(line).starts_with("State:")) {
            info.state = line.substr(line.find(':') + 1);
            size_t first = info.state.find_first_not_of(" \t");
            if (first != std::string::npos) info.state = info.state.substr(first);
            else info.state = "";
        } else if (std::string_view(line).starts_with("VmSize:")) {
            std::string memStr = line.substr(line.find(':') + 1);
            std::stringstream ss(memStr);
            ss >> info.virtualMemory;
        } else if (std::string_view(line).starts_with("VmRSS:")) {
            std::string memStr = line.substr(line.find(':') + 1);
            std::stringstream ss(memStr);
            ss >> info.residentMemory;
        } else if (std::string_view(line).starts_with("PPid:")) {
            std::string ppidStr = line.substr(line.find(':') + 1);
            std::stringstream ss(ppidStr);
            ss >> info.ppid;
        } else if (std::string_view(line).starts_with("Uid:")) {
            std::string uidStr = line.substr(line.find(':') + 1);
            std::stringstream ss(uidStr);
            uint32_t tempUid;
            ss >> tempUid;
            info.uid = tempUid;
        } else if (std::string_view(line).starts_with("Threads:")) {
            std::string threadStr = line.substr(line.find(':') + 1);
            std::stringstream ss(threadStr);
            ss >> info.threadCount;
        }
    }

    // Resolve UID to username
    struct passwd *pw = getpwuid(info.uid);
    if (pw != nullptr) {
        info.username = pw->pw_name;
    }

    // Read cmdline
    std::string cmdlinePath = pidPath + "/cmdline";
    auto cmdlineContentOpt = Utils::readTextFile(cmdlinePath);
    if (cmdlineContentOpt) {
        // cmdline file uses null characters to separate arguments
        std::string rawCmdline = *cmdlineContentOpt;
        std::ranges::replace(rawCmdline, '\0', ' ');
        // Remove trailing space if any
        if (!rawCmdline.empty() && rawCmdline.back() == ' ') {
            rawCmdline.pop_back();
        }
        info.cmdline = rawCmdline;
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
    for (const auto& process : allProcesses) {
        bool nameMatch = true;
        if (filter.nameContains) {
            nameMatch = (process.name.find(*filter.nameContains) != std::string::npos);
        }

        bool userMatch = true;
        if (filter.userFilter) {
            userMatch = (process.username == *filter.userFilter);
        }

        bool stateMatch = true;
        if (filter.stateFilter) {
            // State in /proc/status is usually "S (sleeping)", so we check the first char.
            stateMatch = (!process.state.empty() && process.state[0] == *filter.stateFilter);
        }

        if (nameMatch && userMatch && stateMatch) {
            filteredProcesses.push_back(process);
        }
    }

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
