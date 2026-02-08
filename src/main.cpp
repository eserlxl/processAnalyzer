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
#include "Analyzer.h"
#include "utils.h"

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
    return std::nullopt;
}

// Parses command line arguments.
#include <string_view> // Required for std::span

// ... other includes ...

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
    if ((args.showChildren || args.showOpenFiles) && args.command != "pid") {
        std::cerr << "Error: --children and --open-files are only valid with 'pid' command.\n"; return std::nullopt;
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
        filter.stateFilter = args.stateFilter;

        if (args.command == "list" || args.command == "name" || args.command == "user") {
            processesToDisplay = analyzer.queryProcesses(filter, args.sortBy.value_or(ProcessSortField::pid), args.sortOrder);
        } else if (args.command == "pid") {
            if (!args.pid.has_value()) {
                std::cerr << "Internal error: PID expected.\n";
                return 1;
            }
            int targetPid = 0;
            if (args.pid.has_value()) {
                 targetPid = args.pid.value();
            } else {
                 // Should never happen due to check above
                 return 1;
            }

            auto infoOpt = analyzer.getProcessDetails(targetPid);
            if (!infoOpt) {
                std::cerr << "Error: Process with PID " << targetPid << " not found.\n";
                return 1;
            }
            
            // If --columns is not used, print vertical details and exit.
            if (args.selectedColumns.empty() && !args.outputFormat) {
                printVerticalProcessDetails(*infoOpt);
                if (args.showChildren) {
                    auto children = analyzer.getChildProcesses(targetPid);
                    if (!children.empty()) {
                        std::cout << "\nChildren:\n";
                        printProcessTable(children, getDefaultColumnsForTable(false), args.noTruncateCmdline);
                    } else {
                        std::cout << "\nNo children found.\n";
                    }
                }
                if (args.showOpenFiles) {
                    try {
                        auto fds = analyzer.getOpenFileDescriptors(targetPid);
                        if (!fds.empty()) {
                            std::cout << "\nOpen Files:\n";
                            for (const auto& [fd, path] : fds) {
                                std::cout << "  fd " << std::setw(3) << fd << ": " << path << "\n";
                            }
                        } else {
                            std::cout << "\nNo open files found.\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error reading open files: " << e.what() << "\n";
                    }
                }
                return 0; // Done with pid-specific output
            }
            // Otherwise, add to list for table/csv/json output
            processesToDisplay.push_back(*infoOpt);

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
              << "  --columns <c1,c2,...>       Select columns (pid,ppid,uid,user,name,state,rss,vm,threads,cmdline).\n"
              << "  --format <csv|json>         Set output format.\n"
              << "  --no-truncate-cmdline       Do not truncate the command line in table view.\n"
              << "  --sort-by <field>           Sort by field (pid,name,user,rss,vm,threads,state).\n"
              << "  --desc                      Sort in descending order.\n"
              << "  --state <char>              Filter by process state (e.g., R, S, Z, T, D).\n\n"
              << "PID Specific Options:\n"
              << "  --children                  Show child processes.\n"
              << "  --open-files                Show open files.\n";
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
              << "RSS Memory:        " << info.residentMemory << " KB\n"
              << "Virtual Memory:    " << info.virtualMemory << " KB\n"
              << "Threads:           " << info.threadCount << "\n"
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
        {"cmdline", 40}
    };
    std::map<std::string, int> widths = kDefaultColumnWidths; // Use a mutable copy if needed to adjust widths dynamically later

    // Print header
    for (const auto& col : columns) {
        std::string header = col;
        // Use std::ranges::transform for modernization
        std::ranges::transform(header, header.begin(), ::toupper); 
        if (col == "rss" || col == "vm") header += "(KB)";
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
            std::string value;
            if (col == "pid") value = std::to_string(info.pid);
            else if (col == "ppid") value = std::to_string(info.ppid);
            else if (col == "uid") value = std::to_string(info.uid);
            else if (col == "user") value = info.username;
            else if (col == "name") value = info.name;
            else if (col == "state") value = info.state;
            else if (col == "rss") value = std::to_string(info.residentMemory);
            else if (col == "vm") value = std::to_string(info.virtualMemory);
            else if (col == "threads") value = std::to_string(info.threadCount);
            else if (col == "cmdline") value = info.cmdline;
            
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
            std::string value;
            if (col == "pid") value = std::to_string(info.pid);
            else if (col == "ppid") value = std::to_string(info.ppid);
            else if (col == "uid") value = std::to_string(info.uid);
            else if (col == "user") value = info.username;
            else if (col == "name") value = info.name;
            else if (col == "state") value = info.state;
            else if (col == "rss") value = std::to_string(info.residentMemory);
            else if (col == "vm") value = std::to_string(info.virtualMemory);
            else if (col == "threads") value = std::to_string(info.threadCount);
            else if (col == "cmdline") value = info.cmdline;

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
            ss << "    \"" << col << "\": ";
            if (col == "pid" || col == "ppid" || col == "uid" || col == "rss" || col == "vm" || col == "threads") {
                if (col == "pid") ss << info.pid;
                else if (col == "ppid") ss << info.ppid;
                else if (col == "uid") ss << info.uid;
                else if (col == "rss") ss << info.residentMemory;
                else if (col == "vm") ss << info.virtualMemory;
                else if (col == "threads") ss << info.threadCount;
            } else {
                std::string value;
                if (col == "user") value = info.username;
                else if (col == "name") value = info.name;
                else if (col == "state") value = info.state;
                else if (col == "cmdline") value = info.cmdline;
                // Escape quotes and backslashes
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
