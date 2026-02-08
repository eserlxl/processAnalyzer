// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "cli/Args.h"
#include "utils/String.h"
#include <iostream>
#include <algorithm>
#include <vector>

namespace {
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
}

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
            if (!(stateStr.length() == 1 && std::isalpha(static_cast<unsigned char>(stateStr[0])))) { // Invert condition
                std::cerr << "Error: --state requires a single character (e.g., 'R', 'S').\n";
                return std::nullopt;
            }
            args.stateFilter = static_cast<char>(std::toupper(static_cast<unsigned char>(stateStr[0])));
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
        } else if (arg == "--threads") {
            args.showThreads = true;
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
    if ((args.showChildren || args.showOpenFiles || args.showNetworkConnections || args.showThreads) && args.command != "pid") {
        std::cerr << "Error: --children, --open-files, --threads, and --network are only valid with 'pid' command.\n"; return std::nullopt;
    }
    if (args.ppidFilter.has_value() && args.command == "pid") {
        std::cerr << "Error: --ppid cannot be used with 'pid' command.\n"; return std::nullopt;
    }

    return args;
}
