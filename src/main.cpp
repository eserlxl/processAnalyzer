// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <iostream>
#include <vector>
#include <string>
#include <span>
#include <iomanip>

#include "analyzer/Core.h"
#include "cli/Args.h"
#include "cli/Output.h"

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

        ProcessAnalyzer analyzer("/proc");
        std::vector<ProcessInfo> processesToDisplay;
        ProcessFilter filter;
        
        // Populate filter from args
        if (args.name) filter.nameContains = *args.name;
        if (args.user) filter.userFilter = *args.user;
        if (args.ppidFilter) filter.ppidFilter = args.ppidFilter;
        filter.stateFilter = args.stateFilter;

        if (args.command == "list" || args.command == "name" || args.command == "user") {
            auto processesResult = analyzer.queryProcesses(filter, args.sortBy.value_or(ProcessSortField::pid), args.sortOrder);
            if (processesResult) {
                processesToDisplay = *processesResult;
            } else {
                std::cerr << "Error listing processes: " << processesResult.error().message() << "\n";
                return 1;
            }
        } else if (args.command == "pid") {
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
                                constexpr int kColWidthProto = 8;
                                constexpr int kColWidthAddress = 28;
                                constexpr int kSeparatorWidth = 78;
                                std::cout << "  " << std::left << std::setw(kColWidthProto) << "Proto" << std::setw(kColWidthAddress) << "Local Address" << std::setw(kColWidthAddress) << "Remote Address" << "State\n";
                                std::cout << "  " << std::string(kSeparatorWidth, '-') << "\n";
                                for (const auto& conn : conns) {
                                    std::cout << "  " << std::left 
                                              << std::setw(kColWidthProto) << conn.protocol
                                              << std::setw(kColWidthAddress) << conn.localAddress
                                              << std::setw(kColWidthAddress) << conn.remoteAddress
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
                             constexpr int kTidColumnWidth = 8;
                             constexpr int kThreadSeparatorWidth = 30;
                             std::cout << "  " << std::left << std::setw(kTidColumnWidth) << "TID" << "Name\n";
                             std::cout << "  " << std::string(kThreadSeparatorWidth, '-') << "\n";
                             for(const auto& thread : threads) {
                                 std::cout << "  " << std::left << std::setw(kTidColumnWidth) << thread.tid << thread.name << "\n";
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
        } else {
            printProcessTable(processesToDisplay, columns, args.noTruncateCmdline);
        }
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
