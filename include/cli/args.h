// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <string>
#include <vector>
#include <optional>
#include <span>
#include "analyzer/process_model.h" // For ProcessSortField, SortOrder

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
    bool showThreads = false;
    bool showHelp = false;
    std::optional<int> ppidFilter;       // Stores the PPID value for the --ppid filter.
    bool showNetworkConnections = false; // Flag to indicate if --network option was used.
    std::optional<std::string> configFilePath; // Path to a user-specified configuration file.
};

std::optional<ParsedArguments> parseCommandLine(int argc, std::span<char* const> argv);

#endif // CLI_ARGS_H
