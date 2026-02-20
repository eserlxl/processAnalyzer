// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "cli/output.h"
#include "utils/string.h"
#include "utils/time.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <algorithm>
#include <cassert>

namespace {
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
        if (col == "start-time") {
            return utils::formatTimestamp(info.startTimeUnix).value_or("N/A");
        }
        if (col == "elapsed-time") return info.elapsedTime;
        if (col == "exec-path") return info.executablePath;
        if (col == "nice") return std::to_string(info.priority);
        assert(false && "Unknown column name requested");
    return ""; // Should not happen with valid column names
    }
}

void printUsage() {
    std::cout << "Usage: processAnalyzer [command] [options]\n\n"
              << "A tool for inspecting system processes.\n\n"
              << "Commands:\n"
              << "  list                        List all processes. This is the default command.\n"
              << "  show                        Show details for a specific process (requires --pid).\n"
              << "  pid <pid>                   Show details for a specific process ID.\n"
              << "  name <name>                 Filter processes by name.\n"
              << "  user <user>                 Filter processes by username.\n"
              << "  help                        Show this help message.\n\n"
              << "Options:\n"
              << "  -h, --help                  Show this help message.\n"
              << "  -p, --pid <pid>             Target process ID.\n"
              << "  --brief                     Show a condensed table view.\n"
              << "  --columns <c1,c2,...>       Select columns. Available: pid, ppid, uid, user, name, state, rss, vm, threads, cmdline, start-time, elapsed-time, exec-path, nice.\n"
              << "  --config-file <path>        Path to a configuration file.\n"
              << "  --output <csv|json|table|vertical>  Set output format.\n"
              << "  --no-truncate-cmdline       Do not truncate the command line in table view.\n"
              << "  --sort-by <field>           Sort by field. Available: pid, ppid, name, user, rss, vm, threads, state, start-time.\n"
              << "  --sort-order <asc|desc>     Sort in ascending or descending order.\n"
              << "  --state <char>              Filter by process state (e.g., R, S, Z, T, D).\n"
              << "  --ppid <ppid>               Filter by parent process ID.\n\n"
              << "PID Specific Options:\n"
              << "  --children                  Show child processes.\n"
              << "  --open-files                Show open files.\n"
              << "  --network                   Show network connections.\n"
              << "  --threads                   Show thread information.\n";
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
              << "RSS Memory:        " << info.residentMemory << " KB\n"
              << "Virtual Memory:    " << info.virtualMemory << " KB\n"
              << "Threads:           " << info.threadCount << "\n"
              << "Start Time:        " << utils::formatTimestamp(info.startTimeUnix).value_or("N/A") << "\n"
              << "Elapsed Time:      " << info.elapsedTime << "\n"
              << "Executable Path:   " << info.executablePath << "\n"
              << "Command:           " << info.cmdline << "\n";
}

void printProcessTable(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns, bool noTruncateCmdline) {
    if (processes.empty()) return;

    // Define column widths
    static const std::map<std::string, int> defaultColumnWidths = {
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
        {"start-time", 22},
        {"elapsed-time", 14},
        {"exec-path", 30},
        {"nice", 6}
    };
    std::map<std::string, int> widths = defaultColumnWidths; // Use a mutable copy if needed to adjust widths dynamically later

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
                value = std::string("\"") + utils::replaceAll(value, "\"", "\"\"") + "\"";
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
                value = utils::replaceAll(value, "\\", "\\\\");
                value = utils::replaceAll(value, "\"", "\\\"");
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
