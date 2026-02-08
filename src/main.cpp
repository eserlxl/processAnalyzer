// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <iomanip>
#include <map>
#include <algorithm>
#include <sstream>
#include <span> // Required for std::span
#include "analyzer/Analyzer.h"
#include "utils/Core.h"

// Struct to hold parsed command-line arguments.
struct ParsedArguments {
    std::string command;
    std::optional<int> pid;
    std::optional<std::string> name;
    std::optional<std::string> user;
    std::optional<char> stateFilter;
    std::optional<ProcessSortField> sortBy;
    SortOrder sortOrder = SortOrder::asc; // Default to ascending
    bool briefMode = false;
    std::vector<std::string> selectedColumns;
    bool noTruncateCmdline = false;
    std::optional<std::string> outputFormat;
    bool showChildren = false;
    bool showOpenFiles = false;
    bool showHelp = false;
    std::optional<int> ppidFilter;       // Stores the PPID value for the --ppid filter.
    bool showNetworkConnections = false; // Flag to indicate if --network option was used.
    std::optional<std::string> configFilePath; // Path to a user-specified configuration file.
};

// Forward declarations for functions
void printUsage();
std::vector<std::string> getDefaultColumnsForTable(bool fullDetails);
void printProcessTable(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns, bool noTruncateCmdline);
void printProcessCsv(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns);
void printProcessJson(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns);
void printVerticalProcessDetails(const ProcessInfo& info);

// Helper to convert string to ProcessSortField
std::optional<ProcessSortField> stringToProcessSortField(const std::string& s) {
    std::string lowerS = utils::toLower(s);
    if (lowerS == "pid") return ProcessSortField::pid;
    if (lowerS == "ppid") return ProcessSortField::ppid;
    if (lowerS == "uid") return ProcessSortField::uid;
    if (lowerS == "user") return ProcessSortField::user;
    if (lowerS == "name") return ProcessSortField::name;
    if (lowerS == "state") return ProcessSortField::state;
    if (lowerS == "rss") return ProcessSortField::rss;
    if (lowerS == "vm") return ProcessSortField::vmsize;
    if (lowerS == "threads") return ProcessSortField::threads;
    if (lowerS == "cpu") return ProcessSortField::cpuUsage;
    if (lowerS == "start-time") return ProcessSortField::startTime;
    if (lowerS == "mem-perc") return ProcessSortField::memoryPercentage;
    return std::nullopt;
}

// Helper to get string value of a ProcessInfo field based on column name
std::string getProcessInfoValue(const ProcessInfo& info, const std::string& col) {
    if (col == "pid") return std::to_string(info.pid);
    if (col == "ppid") return std::to_string(info.ppid);
    if (col == "uid") return std::to_string(info.uid);
    if (col == "user") return info.username;
    if (col == "name") return info.name;
    if (col == "state") return info.state;
    if (col == "rss") return std::to_string(info.residentMemory);
    if (col == "vm") return std::to_string(info.virtualMemory);
    if (col == "threads") return std::to_string(info.threadCount);
    if (col == "cmdline") return info.cmdline;
    // New columns for Iteration 13
    if (col == "cpu") {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << info.cpuUsage;
        return ss.str();
    }
    if (col == "start-time") {
        return utils::formatTimestamp(info.startTimeUnix);
    }
    if (col == "elapsed-time") return info.elapsedTime;
    if (col == "mem-perc") {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << info.memoryPercentage;
        return ss.str();
    }
    if (col == "exec-path") return info.executablePath;
    if (col == "nice") return std::to_string(info.priority);
    return ""; // Should not happen with valid column names
}

// Parses command line arguments.

std::optional<ParsedArguments> parseCommandLine(int argc, std::span<char* const> argv) {
    ParsedArguments args;
    if (argc <= 1) {
        args.showHelp = true;
        return args;
    }

    std::vector<std::string> cliArgs;
    for (int i = 1; i < argc; ++i) {
        cliArgs.emplace_back(argv[i]);
    }

    // First argument is usually the command, unless it's a flag.
    if (!cliArgs.empty() && !utils::startsWith(cliArgs[0], "-")) {
        args.command = cliArgs[0];
        cliArgs.erase(cliArgs.begin()); // Consume the command
    } else {
        // Default to 'list' if no command is given but options are.
        args.command = "list";
    }

    if (args.command == "help") {
        args.showHelp = true;
        return args;
    }

    // Parse remaining arguments
    for (size_t i = 0; i < cliArgs.size(); ++i) {
        std::string arg = cliArgs[i];

        if (arg == "--help" || arg == "-h") {
            args.showHelp = true;
            return args;
        }
        
        if (arg == "--state") {
            if (i + 1 >= cliArgs.size()) { 
                std::cerr << "Error: --state requires an argument.\n"; 
                return std::nullopt; 
            }
            const std::string& stateStr = cliArgs[++i];
            if (!(stateStr.length() == 1 && std::isalpha(stateStr[0]))) { // Invert condition
                std::cerr << "Error: --state requires a single character (e.g., 'R', 'S').\n";
                return std::nullopt;
            }
            args.stateFilter = std::toupper(stateStr[0]);
        } else if (arg == "--sort-by") {
            if (i + 1 >= cliArgs.size()) { 
                std::cerr << "Error: --sort-by requires an argument.\n"; 
                return std::nullopt; 
            }
            auto field = stringToProcessSortField(cliArgs[++i]);
            if (!field) {
                std::cerr << "Error: Invalid sort field '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
            args.sortBy = field;
        } else if (arg == "--desc") {
            args.sortOrder = SortOrder::desc;
        } else if (arg == "--brief") {
            args.briefMode = true;
        } else if (arg == "--columns") {
            if (i + 1 >= cliArgs.size()) { std::cerr << "Error: --columns requires an argument.\n"; return std::nullopt; }
            args.selectedColumns = utils::split(cliArgs[++i], ',');
        } else if (arg == "--no-truncate-cmdline") {
            args.noTruncateCmdline = true;
        } else if (arg == "--format") {
            if (i + 1 >= cliArgs.size()) { std::cerr << "Error: --format requires an argument.\n"; return std::nullopt; }
            std::string format = utils::toLower(cliArgs[++i]);
            if (format == "csv" || format == "json") {
                args.outputFormat = format;
            } else {
                std::cerr << "Error: Invalid format '" << cliArgs[i] << "'. Use 'csv' or 'json'.\n";
                return std::nullopt;
            }
        } else if (arg == "--children") {
            args.showChildren = true;
        } else if (arg == "--open-files") {
            args.showOpenFiles = true;
        } else if (arg == "--ppid") {
            if (i + 1 >= cliArgs.size()) { std::cerr << "Error: --ppid requires an argument.\n"; return std::nullopt; }
            constexpr int kBase10 = 10;
            if (auto ppid = utils::toLong(cliArgs[++i], kBase10)) {
                args.ppidFilter = (int)*ppid;
            } else {
                std::cerr << "Error: Invalid PPID '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--network") {
            args.showNetworkConnections = true;
        } else if (arg == "--config") {
            if (i + 1 >= cliArgs.size()) { std::cerr << "Error: --config requires a path.\n"; return std::nullopt; }
            args.configFilePath = cliArgs[++i];
        } else if (utils::startsWith(arg, "-")) {
            std::cerr << "Error: Unknown option '" << arg << "'.\n";
            return std::nullopt;
        } else {
            // Positional arguments
            if (args.command == "pid" && !args.pid) {
                constexpr int kBase10 = 10;
                if (auto pid = utils::toLong(arg, kBase10)) {
                    args.pid = (int)*pid;
                } else {
                    std::cerr << "Error: Invalid PID '" << arg << "'.\n";
                    return std::nullopt;
                }
            } else if (args.command == "name" && !args.name) {
                args.name = arg;
            } else if (args.command == "user" && !args.user) {
                args.user = arg;
            } else {
                std::cerr << "Error: Unexpected argument '" << arg << "'.\n";
                return std::nullopt;
            }
        }
    }

    // Validation
    if (args.command == "pid" && !args.pid) { std::cerr << "Error: 'pid' command requires a PID.\n"; return std::nullopt; }
    if (args.command == "name" && !args.name) { std::cerr << "Error: 'name' command requires a name.\n"; return std::nullopt; }
    if (args.command == "user" && !args.user) { std::cerr << "Error: 'user' command requires a user.\n"; return std::nullopt; }
    if ((args.showChildren || args.showOpenFiles || args.showNetworkConnections) && args.command != "pid") {
        std::cerr << "Error: --children, --open-files, and --network are only valid with 'pid' command.\n"; return std::nullopt;
    }
    if (args.ppidFilter.has_value() && args.command == "pid") {
        std::cerr << "Error: --ppid cannot be used with 'pid' command.\n"; return std::nullopt;
    }

    return args;
}

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

// --- Implementation of Helper Functions ---

void printUsage() {
    std::cout << "Usage: processAnalyzer <command> [options] [args...]\n\n"
              << "A tool for inspecting system processes.\n\n"
              << "Commands:\n"
              << "  list                        List all processes. This is the default command.\n"
              << "  pid <pid>                   Show details for a specific process ID.\n"
              << "  name <name>                 Filter processes by name.\n"
              << "  user <user>                 Filter processes by username.\n"
              << "  help                        Show this help message.\n\n"
              << "Options:\n"
              << "  -h, --help                  Show this help message.\n"
              << "  --brief                     Show a condensed table view.\n"
              << "  --columns <c1,c2,...>       Select columns. Available: pid, ppid, uid, user, name, state, rss, vm, threads, cmdline, cpu, start-time, elapsed-time, mem-perc, exec-path, nice.\n"
              << "  --config <path>             Path to a configuration file.\n"
              << "  --format <csv|json>         Set output format.\n"
              << "  --no-truncate-cmdline       Do not truncate the command line in table view.\n"
              << "  --sort-by <field>           Sort by field. Available: pid, ppid, name, user, rss, vm, threads, state, cpu, start-time, mem-perc.\n"
              << "  --desc                      Sort in descending order.\n"
              << "  --state <char>              Filter by process state (e.g., R, S, Z, T, D).\n"
              << "  --ppid <ppid>               Filter by parent process ID.\n\n"
              << "PID Specific Options:\n"
              << "  --children                  Show child processes.\n"
              << "  --open-files                Show open files.\n"
              << "  --network                   Show network connections.\n";
}


std::vector<std::string> getDefaultColumnsForTable(bool fullDetails) {
    if (fullDetails) {
        return {"pid", "user", "name", "state", "rss", "vm", "threads", "cmdline"};
    }
    return {"pid", "user", "name", "state", "rss"};
}

void printVerticalProcessDetails(const ProcessInfo& info) {
    std::cout << "PID:               " << info.pid << "\n"
              << "PPID:              " << info.ppid << "\n"
              << "UID:               " << info.uid << "\n"
              << "User:              " << info.username << "\n"
              << "Name:              " << info.name << "\n"
              << "State:             " << info.state << "\n"
              << "Nice Value:        " << info.priority << "\n"
              << "CPU Usage:         " << std::fixed << std::setprecision(2) << info.cpuUsage << " %\n"
              << "Memory Usage:      " << std::fixed << std::setprecision(2) << info.memoryPercentage << " %\n"
              << "RSS Memory:        " << info.residentMemory << " KB\n"
              << "Virtual Memory:    " << info.virtualMemory << " KB\n"
              << "Threads:           " << info.threadCount << "\n"
              << "Start Time:        " << utils::formatTimestamp(info.startTimeUnix) << "\n"
              << "Elapsed Time:      " << info.elapsedTime << "\n"
              << "Executable Path:   " << info.executablePath << "\n"
              << "Command:           " << info.cmdline << "\n";
}

void printProcessTable(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns, bool noTruncateCmdline) {
    if (processes.empty()) return;

    // Define column widths
    static const std::map<std::string, int> kDefaultColumnWidths = {
        {"pid", 8},
        {"ppid", 8},
        {"uid", 8},
        {"user", 15},
        {"name", 25},
        {"state", 12},
        {"rss", 10},
        {"vm", 10},
        {"threads", 8},
        {"cmdline", 40},
        {"cpu", 8},
        {"start-time", 22},
        {"elapsed-time", 14},
        {"mem-perc", 10},
        {"exec-path", 30},
        {"nice", 6}
    };
    std::map<std::string, int> widths = kDefaultColumnWidths; // Use a mutable copy if needed to adjust widths dynamically later

    // Print header
    for (const auto& col : columns) {
        std::string header = col;
        // Use std::ranges::transform for modernization
        std::ranges::transform(header, header.begin(), ::toupper); 
        if (col == "rss" || col == "vm") header += "(KB)";
        if (col == "cpu" || col == "mem-perc") header += "(%)";
        std::cout << std::left << std::setw(widths[col]) << header;
    }
    std::cout << std::endl;

    // Print separator
    for (const auto& col : columns) {
        std::cout << std::string(widths[col], '-') ;
    }
    std::cout << std::endl;

    // Print rows
    for (const auto& info : processes) {
        for (const auto& col : columns) {
            std::string value = getProcessInfoValue(info, col);
            
            int width = widths[col];
            if (col == "cmdline" && noTruncateCmdline) {
                 std::cout << std::left << value;
            } else {
                if (value.length() > (size_t)width) {
                    value = value.substr(0, width - 1) + "~";
                }
                std::cout << std::left << std::setw(width) << value;
            }
        }
        std::cout << std::endl;
    }
}

void printProcessCsv(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns) {
    // Header
    std::cout << utils::join(columns, ",") << std::endl;

    // Rows
    for (const auto& info : processes) {
        std::vector<std::string> values;
        for (const auto& col : columns) {
            std::string value = getProcessInfoValue(info, col);

            // Quote if necessary
            if (value.find(',') != std::string::npos || value.find('"') != std::string::npos) {
                value = std::string("\"") + utils::replace(value, "\"", "\"\"") + "\"";
            }
            values.push_back(value);
        }
        std::cout << utils::join(values, ",") << std::endl;
    }
}

void printProcessJson(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns) {
    std::cout << "[\n";
    for (size_t i = 0; i < processes.size(); ++i) {
        const auto& info = processes[i];
        std::cout << "  {\n";
        
        std::vector<std::string> pairs;
        for (const auto& col : columns) {
            std::stringstream ss;
            std::string value = getProcessInfoValue(info, col);

            ss << "    \"" << col << "\": ";
            if (col == "pid" || col == "ppid" || col == "uid" || col == "rss" || col == "vm" || col == "threads") {
                ss << value; // Numerical values as is
            } else {
                // Escape quotes and backslashes for string values
                value = utils::replace(value, "\\", "\\\\");
                value = utils::replace(value, "\"", "\\\"");
                ss << "\"" << value << "\"";
            }
            pairs.push_back(ss.str());
        }
        std::cout << utils::join(pairs, ",\n");
        std::cout << "\n  }";
        if (i < processes.size() - 1) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "]\n";
}
