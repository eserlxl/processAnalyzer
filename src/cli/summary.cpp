// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "cli/summary.h"
#include "cli/output.h" // jsonEscape

#include <iostream>
#include <iomanip>
#include <sstream>

ProcessSummary summarizeProcesses(const std::vector<ProcessInfo>& processes) {
    ProcessSummary summary;
    summary.processCount = processes.size();
    for (const auto& info : processes) {
        ++summary.countByState[info.state];
        if (info.state == "Z") {
            ++summary.zombieCount;
        }
        summary.totalThreads += info.threadCount;
        summary.totalResidentMemoryKB += info.residentMemory;
        summary.totalVirtualMemoryKB += info.virtualMemory;
    }
    return summary;
}

void printProcessSummary(const ProcessSummary& summary, bool asJson) {
    if (asJson) {
        std::ostringstream byState;
        byState << "{";
        bool first = true;
        for (const auto& [state, count] : summary.countByState) {
            if (!first) { byState << ", "; }
            first = false;
            byState << "\"" << jsonEscape(state) << "\": " << count;
        }
        byState << "}";
        std::cout << "{\"process_count\": " << summary.processCount
                  << ", \"by_state\": " << byState.str()
                  << ", \"zombie_count\": " << summary.zombieCount
                  << ", \"total_threads\": " << summary.totalThreads
                  << ", \"total_resident_kb\": " << summary.totalResidentMemoryKB
                  << ", \"total_virtual_kb\": " << summary.totalVirtualMemoryKB
                  << "}\n";
        return;
    }

    constexpr int labelWidth = 18;
    constexpr int stateColWidth = 6;
    constexpr unsigned long kbPerMib = 1024;
    std::cout << "=== Process Summary ===\n";
    std::cout << std::left << std::setw(labelWidth) << "Total processes:" << summary.processCount << "\n";
    std::cout << "\nBy state:\n";
    for (const auto& [state, count] : summary.countByState) {
        std::cout << "  " << std::left << std::setw(stateColWidth) << state << count << "\n";
    }
    std::cout << "\n" << std::left << std::setw(labelWidth) << "Zombies:" << summary.zombieCount << "\n";
    std::cout << std::left << std::setw(labelWidth) << "Total threads:" << summary.totalThreads << "\n";
    std::cout << std::left << std::setw(labelWidth) << "Total RSS:"
              << summary.totalResidentMemoryKB << " KB ("
              << summary.totalResidentMemoryKB / static_cast<long long>(kbPerMib) << " MiB)\n";
    std::cout << std::left << std::setw(labelWidth) << "Total VM:"
              << summary.totalVirtualMemoryKB << " KB ("
              << summary.totalVirtualMemoryKB / static_cast<long long>(kbPerMib) << " MiB)\n";
}
