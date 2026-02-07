// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <iostream>
#include <string>
#include <vector>
#include <optional>     // For std::optional
#include <iomanip>      // For std::setw
#include "Analyzer.h"

void printUsage() {
    std::cout << "Usage: processAnalyzer [option]\n"
              << "Options:\n"
              << "  list            List all processes\n"
              << "  pid <number>    Show details for a specific PID\n"
              << "  name <name>     Show processes by name\n" // New option
              << "  user <username> Show processes by user\n" // New option
              << "  help            Show this help message\n";
}

void printProcessTable(const std::vector<ProcessInfo>& processes, bool fullDetails) {
    if (fullDetails) {
        std::cout << std::left << std::setw(8) << "PID" 
                  << std::setw(15) << "User"
                  << std::setw(20) << "Name" 
                  << std::setw(20) << "State" 
                  << std::setw(10) << "RSS(KB)" 
                  << std::setw(10) << "VM(KB)"
                  << std::setw(10) << "Threads"
                  << "Cmdline" << std::endl;
        std::cout << std::string(103, '-') << std::endl;

        for (const auto& info : processes) {
            std::cout << std::left << std::setw(8) << info.pid 
                      << std::setw(15) << info.username.substr(0, 14)
                      << std::setw(20) << info.name.substr(0, 19) 
                      << std::setw(20) << info.state.substr(0, 19) 
                      << std::setw(10) << info.residentMemory 
                      << std::setw(10) << info.virtualMemory
                      << std::setw(10) << info.threadCount
                      << info.cmdline.substr(0, 40) << (info.cmdline.length() > 40 ? "..." : "") << std::endl;
        }
    } else {
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
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string command = argv[1];
    ProcessAnalyzer analyzer("/proc"); // Explicitly pass proc path

    if (command == "list") {
        auto processes = analyzer.snapshot();
        printProcessTable(processes, true);
    } else if (command == "pid") {
        if (argc < 3) {
            std::cerr << "Error: PID required.\n";
            return 1;
        }
        try {
            int pid = std::stoi(argv[2]);
            std::optional<ProcessInfo> infoOpt = analyzer.getProcessDetails(pid); // Handle optional
            if (infoOpt) {
                const ProcessInfo& info = *infoOpt;
                std::cout << "PID: " << info.pid << "\n"
                          << "PPID: " << info.ppid << "\n"
                          << "UID: " << info.uid << "\n"
                          << "User: " << info.username << "\n"
                          << "Name: " << info.name << "\n"
                          << "State: " << info.state << "\n"
                          << "Resident Memory: " << info.residentMemory << " KB\n" // Renamed field
                          << "Virtual Memory: " << info.virtualMemory << " KB\n"
                          << "Threads: " << info.threadCount << "\n"
                          << "Cmdline: " << info.cmdline << "\n";
            } else {
                std::cerr << "Error: Process with PID " << pid << " not found or accessible.\n";
            }
        } catch (...) {
            std::cerr << "Error: Invalid PID format.\n";
            return 1;
        }
    } else if (command == "name") { // New command
        if (argc < 3) {
            std::cerr << "Error: Process name required.\n";
            return 1;
        }
        std::string processName = argv[2];
        auto processes = analyzer.getProcessesByName(processName);
        if (processes.empty()) {
            std::cout << "No processes found with name: " << processName << "\n";
        } else {
            std::cout << "Processes with name '" << processName << "':\n";
            printProcessTable(processes, false);
        }
    } else if (command == "user") { // New command
        if (argc < 3) {
            std::cerr << "Error: Username required.\n";
            return 1;
        }
        std::string username = argv[2];
        auto processes = analyzer.getProcessesByUser(username);
        if (processes.empty()) {
            std::cout << "No processes found for user: " << username << "\n";
        } else {
            std::cout << "Processes for user '" << username << "':\n";
            printProcessTable(processes, false);
        }
    }
    else {
        printUsage();
    }

    return 0;
}
