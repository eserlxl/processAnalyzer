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

namespace fs = std::filesystem;

ProcessAnalyzer::ProcessAnalyzer(std::string_view procPath) : procPath(procPath) {}

std::vector<int> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return {1, 100, 200}; 
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
    } catch (const fs::filesystem_error&) {
        // Handle permission errors etc.
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
    std::string statusContent = *statusContentOpt;
    
    std::vector<std::string> lines = Utils::split(statusContent, '\n');
    for (const auto& line : lines) {
        if (line.rfind("Name:", 0) == 0) {
            info.name = line.substr(line.find(':') + 1);
            size_t first = info.name.find_first_not_of(" \t");
            if (first != std::string::npos) info.name = info.name.substr(first);
            else info.name = "";
        } else if (line.rfind("State:", 0) == 0) {
            info.state = line.substr(line.find(':') + 1);
            size_t first = info.state.find_first_not_of(" \t");
            if (first != std::string::npos) info.state = info.state.substr(first);
            else info.state = "";
        } else if (line.rfind("VmSize:", 0) == 0) {
            std::string memStr = line.substr(line.find(':') + 1);
            std::stringstream ss(memStr);
            ss >> info.virtualMemory;
        } else if (line.rfind("VmRSS:", 0) == 0) {
            std::string memStr = line.substr(line.find(':') + 1);
            std::stringstream ss(memStr);
            ss >> info.residentMemory;
        } else if (line.rfind("PPid:", 0) == 0) {
            std::string ppidStr = line.substr(line.find(':') + 1);
            std::stringstream ss(ppidStr);
            ss >> info.ppid;
        } else if (line.rfind("Uid:", 0) == 0) {
            std::string uidStr = line.substr(line.find(':') + 1);
            std::stringstream ss(uidStr);
            uint32_t tempUid;
            ss >> tempUid;
            info.uid = tempUid;
        } else if (line.rfind("Threads:", 0) == 0) {
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
        std::replace(rawCmdline.begin(), rawCmdline.end(), '\0', ' ');
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

std::vector<ProcessInfo> ProcessAnalyzer::findProcesses(ProcessPredicate predicate) const {
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

void ProcessAnalyzer::printAllProcesses() const {
    auto processes = snapshot();
    std::cout << std::left << std::setw(8) << "PID" 
              << std::setw(15) << "User"
              << std::setw(20) << "Name" 
              << std::setw(20) << "State" 
              << std::setw(10) << "RSS(KB)" << std::endl;
    std::cout << std::string(73, '-') << std::endl;

    for (const auto& info : processes) {
        std::cout << std::left << std::setw(8) << info.pid 
                  << std::setw(15) << info.username.substr(0, 14)
                  << std::setw(20) << info.name.substr(0, 19) 
                  << std::setw(20) << info.state.substr(0, 19) 
                  << std::setw(10) << info.residentMemory << std::endl;
    }
}
