// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <iostream>
#include <span>
#include <iomanip>
#include <filesystem>

#include "analyzer/core.h"
#include "cli/args.h"
#include "cli/output.h"
#include "utils/time.h"

int main(int argc, char* argv[]) {
    try {
        auto argsOpt = parseCommandLine(argc, std::span(argv, argc));
        if (!argsOpt) {
            return 1;
        }
        ParsedArguments args = *argsOpt;

        if (args.showHelp) {
            printUsage();
            return 0;
        }

        ProcessAnalyzer analyzer(std::filesystem::path("/proc"));
        std::vector<ProcessInfo> processesToDisplay;
        ProcessFilter filter;
        
        // Populate filter from args
        if (args.name) filter.nameContains = *args.name;
        if (args.user) filter.userFilter = *args.user;
        if (args.ppidFilter) filter.ppidFilter = args.ppidFilter;
        filter.stateFilter = args.stateFilter;
        filter.minResidentMemoryKB = args.minRssKb;
        filter.maxResidentMemoryKB = args.maxRssKb;
        filter.minThreads = args.minThreads;
        filter.maxThreads = args.maxThreads;

        if (args.command == "system") {
            constexpr int labelWidth = 20;
            // System info
            auto infoResult = analyzer.getSystemInfo();
            if (infoResult) {
                const auto& info = *infoResult;
                std::cout << "=== System Information ===\n";
                std::cout << std::left << std::setw(labelWidth) << "Hostname:" << info.hostname << "\n";
                std::cout << std::left << std::setw(labelWidth) << "OS:" << info.osName << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Kernel:" << info.kernelVersion << "\n";
                long long uptimeSecs = std::chrono::duration_cast<std::chrono::seconds>(info.uptime).count();
                std::string uptimeStr = utils::formatElapsedTime(uptimeSecs).value_or("N/A");
                std::cout << std::left << std::setw(labelWidth) << "Uptime:" << uptimeStr << "\n";
            }
            // Load average
            auto loadResult = analyzer.getSystemLoadAverage();
            if (loadResult) {
                const auto& avg = *loadResult;
                std::cout << "\n=== Load Average ===\n";
                std::cout << std::left << std::setw(labelWidth) << "1 min:" << avg.oneMin << "\n";
                std::cout << std::left << std::setw(labelWidth) << "5 min:" << avg.fiveMin << "\n";
                std::cout << std::left << std::setw(labelWidth) << "15 min:" << avg.fifteenMin << "\n";
            }
            // Memory
            auto memResult = analyzer.getSystemMemoryInfo();
            if (memResult) {
                const auto& mem = *memResult;
                constexpr unsigned long kbPerMib = 1024;
                std::cout << "\n=== Memory (MiB) ===\n";
                std::cout << std::left << std::setw(labelWidth) << "Total:" << mem.memTotal / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Free:" << mem.memFree / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Available:" << mem.memAvailable / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Buffers:" << mem.buffers / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Cached:" << mem.cached / kbPerMib << "\n";
                if (mem.swapTotal > 0) {
                    std::cout << std::left << std::setw(labelWidth) << "Swap Total:" << mem.swapTotal / kbPerMib << "\n";
                    std::cout << std::left << std::setw(labelWidth) << "Swap Free:" << mem.swapFree / kbPerMib << "\n";
                }
            }
            // Disk usage
            auto diskResult = analyzer.getSystemDiskUsage();
            if (diskResult && !diskResult->empty()) {
                constexpr unsigned long long bytesPerGib = 1024ULL * 1024 * 1024;
                std::cout << "\n=== Disk Usage ===\n";
                constexpr int devWidth = 20;
                constexpr int mpWidth = 24;
                constexpr int numWidth = 10;
                std::cout << std::left << std::setw(devWidth) << "Device"
                          << std::setw(mpWidth) << "Mount Point"
                          << std::right << std::setw(numWidth) << "Total(GiB)"
                          << std::setw(numWidth) << "Free(GiB)" << "\n";
                std::cout << std::string(devWidth + mpWidth + (numWidth * 2), '-') << "\n";
                for (const auto& mp : *diskResult) {
                    std::cout << std::left << std::setw(devWidth) << mp.device
                              << std::setw(mpWidth) << mp.mountPoint
                              << std::right << std::setw(numWidth)
                              << std::fixed << std::setprecision(1)
                              << static_cast<double>(mp.totalSpaceBytes) / static_cast<double>(bytesPerGib)
                              << std::setw(numWidth)
                              << static_cast<double>(mp.freeSpaceBytes) / static_cast<double>(bytesPerGib)
                              << "\n";
                }
            }
            return 0;
        }
        if (args.command == "list" || args.command == "name" || args.command == "user") {
            auto processesResult = analyzer.queryProcesses(filter, args.sortBy.value_or(ProcessSortField::pid), args.sortOrder);
            if (processesResult) {
                processesToDisplay = *processesResult;
            } else {
                std::cerr << "Error listing processes: " << processesResult.error().message() << "\n";
                return 1;
            }
        } else if (args.command == "pid" || args.command == "show") {
            if (!args.pid.has_value()) {
                std::cerr << "Internal error: PID expected.\n";
                return 1;
            }
            int targetPid = *args.pid;

            auto infoResult = analyzer.getProcessDetails(targetPid);
            if (!infoResult) {
                std::cerr << "Error: " << infoResult.error().message() << "\n";
                return 1;
            }
            
            // If --columns is not used, print vertical details and exit.
            if (args.selectedColumns.empty() && !args.outputFormat) {
                printVerticalProcessDetails(*infoResult);
                if (args.showChildren) {
                    auto childrenResult = analyzer.getChildProcesses(targetPid);
                    if (childrenResult) {
                        auto& children = *childrenResult;
                        if (!children.empty()) {
                            std::cout << "\nChildren:\n";
                            printProcessTable(children, getDefaultColumnsForTable(false), args.noTruncateCmdline);
                        } else {
                            std::cout << "\nNo children found.\n";
                        }
                    } else {
                        std::cerr << "\nError getting children: " << childrenResult.error().message() << "\n";
                    }
                }
                if (args.showOpenFiles) {
                    try {
                        auto fdsResult = analyzer.getProcessOpenFileDetails(targetPid);
                        if (fdsResult) {
                            auto& fds = *fdsResult;
                            if (!fds.empty()) {
                                std::cout << "\nOpen Files:\n";
                                for (const auto& fdInfo : fds) {
                                    std::cout << "  fd " << std::setw(3) << fdInfo.fd << ": " << fdInfo.path << "\n";
                                }
                            } else {
                                std::cout << "\nNo open files found.\n";
                            }
                        } else {
                            std::cerr << "\nError reading open files: " << fdsResult.error().message() << "\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error reading open files: " << e.what() << "\n";
                    }
                }
                if (args.showNetworkConnections) {
                    try {
                        auto connsResult = analyzer.getNetworkConnections(targetPid);
                        if (connsResult) {
                            auto& conns = *connsResult;
                            if (!conns.empty()) {
                                std::cout << "\nNetwork Connections:\n";
                                // Header
                                constexpr int colWidthProto = 8;
                                constexpr int colWidthAddress = 28;
                                constexpr int separatorWidth = 78;
                                std::cout << "  " << std::left << std::setw(colWidthProto) << "Proto" << std::setw(colWidthAddress) << "Local Address" << std::setw(colWidthAddress) << "Remote Address" << "State\n";
                                std::cout << "  " << std::string(separatorWidth, '-') << "\n";
                                for (const auto& conn : conns) {
                                    std::cout << "  " << std::left 
                                              << std::setw(colWidthProto) << conn.protocol
                                              << std::setw(colWidthAddress) << conn.localAddress
                                              << std::setw(colWidthAddress) << conn.remoteAddress
                                              << conn.state << "\n";
                                }
                            } else {
                                std::cout << "\nNo network connections found.\n";
                            }
                        } else {
                            std::cerr << "\nError reading network connections: " << connsResult.error().message() << "\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error reading network connections: " << e.what() << "\n";
                    }
                }
                if (args.showThreads) {
                    auto threadsResult = analyzer.getProcessThreads(targetPid);
                    if (threadsResult) {
                         auto& threads = *threadsResult;
                         if (!threads.empty()) {
                             std::cout << "\nThreads:\n";
                             // Header and Loop
                             constexpr int tidColumnWidth = 8;
                             constexpr int threadSeparatorWidth = 30;
                             std::cout << "  " << std::left << std::setw(tidColumnWidth) << "TID" << "Name\n";
                             std::cout << "  " << std::string(threadSeparatorWidth, '-') << "\n";
                             for(const auto& thread : threads) {
                                 std::cout << "  " << std::left << std::setw(tidColumnWidth) << thread.tid << thread.name << "\n";
                             }
                         } else {
                             std::cout << "\nNo threads found.\n";
                         }
                    } else {
                        std::cerr << "\nError reading threads: " << threadsResult.error().message() << "\n";
                    }
                }
                return 0; // Done with pid-specific output
            }
            // Otherwise, add to list for table/csv/json output
            processesToDisplay.push_back(*infoResult);

        } else {
            std::cerr << "Error: Unknown command '" << args.command << "'.\n";
            printUsage();
            return 1;
        }
        
        // Determine columns for output
        std::vector<std::string> columns = args.selectedColumns;
        if (columns.empty()) {
            columns = getDefaultColumnsForTable(args.command == "list" && !args.briefMode);
        }

        // Output formatting
        if (processesToDisplay.empty() && args.command != "pid") {
            std::cout << "No processes found matching criteria.\n";
        } else if (args.outputFormat == "csv") {
            printProcessCsv(processesToDisplay, columns);
        } else if (args.outputFormat == "json") {
            printProcessJson(processesToDisplay, columns);
        } else if (args.outputFormat == "vertical") {
            for (size_t i = 0; i < processesToDisplay.size(); ++i) {
                printVerticalProcessDetails(processesToDisplay[i]);
                if (i + 1 < processesToDisplay.size()) {
                    std::cout << "\n";
                }
            }
        } else {
            printProcessTable(processesToDisplay, columns, args.noTruncateCmdline);
        }
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
