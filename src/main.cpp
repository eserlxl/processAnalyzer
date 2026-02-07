// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <iostream>
#include <string>
#include "process_analyzer.h"

void printUsage() {
    std::cout << "Usage: processAnalyzer [option]\n"
              << "Options:\n"
              << "  list            List all processes\n"
              << "  pid <number>    Show details for a specific PID\n"
              << "  help            Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string command = argv[1];
    ProcessAnalyzer analyzer;

    if (command == "list") {
        analyzer.printAllProcesses();
    } else if (command == "pid") {
        if (argc < 3) {
            std::cerr << "Error: PID required.\n";
            return 1;
        }
        try {
            int pid = std::stoi(argv[2]);
            ProcessInfo info = analyzer.getProcessDetails(pid);
            std::cout << "PID: " << info.pid << "\n"
                      << "Name: " << info.name << "\n"
                      << "State: " << info.state << "\n"
                      << "Memory: " << info.memory_usage << " KB\n";
        } catch (...) {
            std::cerr << "Error: Invalid PID format.\n";
            return 1;
        }
    } else {
        printUsage();
    }

    return 0;
}
