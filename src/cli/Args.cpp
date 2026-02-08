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
        if (lowerS == "mem-perc" || lowerS == "mem") return ProcessSortField::memoryPercentage;
        return std::nullopt;
    }
}

std::optional<ParsedArguments> parseCommandLine(int argc, std::span<char* const> argv) {
    ParsedArguments args;
    
    std::vector<std::string> cliArgs;
    for (int i = 1; i < argc; ++i) {
        cliArgs.emplace_back(argv[i]);
    }

    // Determine the command (e.g., "list", "show")
    if (!cliArgs.empty()) {
        std::string potentialCommand = cliArgs[0];
        if (!utils::startsWith(potentialCommand, "-")) { // It's a positional argument, so it could be a command
            if (potentialCommand == "list" || potentialCommand == "show") {
                args.command = potentialCommand;
                cliArgs.erase(cliArgs.begin()); // Consume the command
            } else if (potentialCommand == "help") {
                args.showHelp = true;
                return args;
            } else {
                // If it's not a recognized command, it's an error.
                std::cerr << "Error: Unknown command '" << potentialCommand << "'.\n";
                return std::nullopt;
            }
        }
    }

    // If no explicit command like "list" or "show" was given, default to "list".
    // This happens if cliArgs[0] was a flag, or cliArgs was empty after the command was consumed,
    // or if cliArgs was initially empty (argc <= 1 handled earlier for --help).
    if (args.command.empty() && !args.showHelp) {
        args.command = "list";
    }

    // Parse remaining arguments
    for (size_t i = 0; i < cliArgs.size(); ++i) {
        std::string arg = cliArgs[i];

        if (arg == "--help" || arg == "-h") {
            args.showHelp = true;
            return args;
        } else if (arg == "--pid" || arg == "-p") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --pid requires an argument.\n";
                return std::nullopt;
            }
            constexpr int kBase10 = 10;
            if (auto pid = utils::toLong(cliArgs[++i], kBase10)) {
                args.pid = (int)*pid;
            } else {
                std::cerr << "Error: Invalid PID '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--name") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --name requires an argument.\n";
                return std::nullopt;
            }
            args.name = cliArgs[++i];
        } else if (arg == "--user" || arg == "-u") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --user requires an argument.\n";
                return std::nullopt;
            }
            args.user = cliArgs[++i];
        } else if (arg == "--state" || arg == "-s") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --state requires an argument.\n";
                return std::nullopt;
            }
            const std::string& stateStr = cliArgs[++i];
            if (!(stateStr.length() == 1 && std::isalpha(static_cast<unsigned char>(stateStr[0])))) {
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
        } else if (arg == "--sort-order") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --sort-order requires an argument (asc or desc).\n";
                return std::nullopt;
            }
            std::string orderStr = utils::toLower(cliArgs[++i]);
            if (orderStr == "asc") {
                args.sortOrder = SortOrder::asc;
            } else if (orderStr == "desc") {
                args.sortOrder = SortOrder::desc;
            } else {
                std::cerr << "Error: Invalid sort order '" << cliArgs[i] << "'. Use 'asc' or 'desc'.\n";
                return std::nullopt;
            }
        } else if (arg == "--brief" || arg == "-b") {
            args.briefMode = true;
        } else if (arg == "--columns") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --columns requires an argument.\n";
                return std::nullopt;
            }
            args.selectedColumns = utils::split(cliArgs[++i], ',');
        } else if (arg == "--no-truncate-cmdline") {
            args.noTruncateCmdline = true;
        } else if (arg == "--output" || arg == "-o") { // Renamed from --format to --output
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --output requires an argument.\n";
                return std::nullopt;
            }
            std::string format = utils::toLower(cliArgs[++i]);
            if (format == "csv" || format == "json" || format == "table" || format == "vertical") {
                args.outputFormat = format;
            } else {
                std::cerr << "Error: Invalid output format '" << cliArgs[i] << "'. Use 'csv', 'json', 'table', or 'vertical'.\n";
                return std::nullopt;
            }
        } else if (arg == "--children") {
            args.showChildren = true;
        } else if (arg == "--threads") {
            args.showThreads = true;
        } else if (arg == "--open-files") {
            args.showOpenFiles = true;
        } else if (arg == "--ppid") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --ppid requires an argument.\n";
                return std::nullopt;
            }
            constexpr int kBase10 = 10;
            if (auto ppid = utils::toLong(cliArgs[++i], kBase10)) {
                args.ppidFilter = (int)*ppid;
            } else {
                std::cerr << "Error: Invalid PPID '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--network") {
            args.showNetworkConnections = true;
        } else if (arg == "--config-file") { // Renamed from --config to --config-file
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --config-file requires a path.\n";
                return std::nullopt;
            }
            args.configFilePath = cliArgs[++i];
        } else if (utils::startsWith(arg, "-")) {
            // This catches any unknown options that start with '-'
            std::cerr << "Error: Unknown option '" << arg << "'.\n";
            return std::nullopt;
        } else {
            // If it's not an option, and the command hasn't been set yet, it's the command.
            // If the command has already been set, then this is an unexpected argument.
            if (args.command.empty()) {
                args.command = arg;
            } else {
                std::cerr << "Error: Unexpected argument '" << arg << "'.\n";
                return std::nullopt;
            }
        }
    }

    // Final validation
    if (args.command.empty() && !args.showHelp) {
        std::cerr << "Error: No command provided. Use 'list' or 'show'.\n";
        return std::nullopt;
    }

    if (args.command == "show" && !args.pid.has_value()) {
        std::cerr << "Error: 'show' command requires a PID using --pid or -p.\n";
        return std::nullopt;
    }

    // --children, --open-files, --threads, --network are only valid with 'show' command
    if ((args.showChildren || args.showOpenFiles || args.showNetworkConnections || args.showThreads) && args.command != "show") {
        std::cerr << "Error: --children, --open-files, --threads, and --network are only valid with the 'show' command.\n";
        return std::nullopt;
    }

    // --ppid cannot be used with 'show' command (as show is for a single PID)
    if (args.ppidFilter.has_value() && args.command == "show") {
        std::cerr << "Error: --ppid cannot be used with 'show' command.\n";
        return std::nullopt;
    }

    return args;
}
