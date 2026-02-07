// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

ProcessAnalyzer::ProcessAnalyzer() : proc_dir("/proc") {}

std::vector<int> ProcessAnalyzer::getPids() {
    std::vector<int> pids;
    // Check if /proc exists (Linux specific)
    if (!fs::exists(proc_dir)) {
        // Fallback for non-Linux or testing: return some mock PIDs
        return {1, 100, 200}; 
    }

    try {
        for (const auto& entry : fs::directory_iterator(proc_dir)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (Utils::isInteger(filename)) {
                    pids.push_back(std::stoi(filename));
                }
            }
        }
    } catch (const std::exception& e) {
        // Handle permission errors etc.
    }
    return pids;
}

ProcessInfo ProcessAnalyzer::getProcessDetails(int pid) {
    ProcessInfo info;
    info.pid = pid;
    info.name = "Unknown";
    info.state = "?";
    info.memory_usage = 0;

    std::string statusPath = proc_dir + "/" + std::to_string(pid) + "/status";
    
    // Simple mock if /proc doesn't exist
    if (!fs::exists(proc_dir)) {
        info.name = "MockProcess";
        info.state = "Running";
        info.memory_usage = 1024 * pid;
        return info;
    }

    auto result = Utils::readTextFile(statusPath);
    if (!result) return info;
    std::string content = *result;

    std::vector<std::string> lines = Utils::split(content, '\n');
    for (const auto& line : lines) {
        if (line.rfind("Name:", 0) == 0) {
            info.name = line.substr(6); 
            size_t first = info.name.find_first_not_of(" \t");
            if (first != std::string::npos) info.name = info.name.substr(first);
        }
        else if (line.rfind("State:", 0) == 0) {
             info.state = line.substr(7);
             size_t first = info.state.find_first_not_of(" \t");
             if (first != std::string::npos) info.state = info.state.substr(first);
        }
        else if (line.rfind("VmSize:", 0) == 0) {
            std::string memStr = line.substr(8); 
            std::stringstream ss(memStr);
            ss >> info.memory_usage;
        }
    }
    return info;
}

void ProcessAnalyzer::printAllProcesses() {
    auto pids = getPids();
    std::cout << std::left << std::setw(8) << "PID" 
              << std::setw(20) << "Name" 
              << std::setw(20) << "State" 
              << std::setw(10) << "Mem(KB)" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (int pid : pids) {
        ProcessInfo info = getProcessDetails(pid);
        std::cout << std::left << std::setw(8) << info.pid 
                  << std::setw(20) << info.name.substr(0, 19) 
                  << std::setw(20) << info.state.substr(0, 19) 
                  << std::setw(10) << info.memory_usage << std::endl;
    }
}
