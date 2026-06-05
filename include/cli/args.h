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
    std::optional<int> ppidFilter;            // Stores the PPID value for the --ppid filter.
    bool showNetworkConnections = false;      // Flag to indicate if --network option was used.
    std::optional<uint16_t> networkPortFilter; // Local port for --network <port> list filter.
    std::optional<long long> minRssKb;        // Minimum resident memory in KB (--min-rss).
    std::optional<long long> maxRssKb;        // Maximum resident memory in KB (--max-rss).
    std::optional<long> minThreads;           // Minimum thread count (--min-threads).
    std::optional<long> maxThreads;           // Maximum thread count (--max-threads).
    std::optional<int> uidFilter;             // Filter by numeric UID (--uid).
    std::optional<std::string> cmdlineFilter; // Filter by cmdline substring (--cmdline).
    std::optional<long long> minVmKb;         // Minimum virtual memory in KB (--min-vm).
    std::optional<long long> maxVmKb;         // Maximum virtual memory in KB (--max-vm).
    std::optional<int> minPriority;           // Minimum process priority (--min-priority).
    std::optional<int> maxPriority;           // Maximum process priority (--max-priority).
    bool showEnv = false;                     // Show environment variables (--env).
    bool showMemoryMaps = false;              // Show memory maps (--maps).
    bool showLimits = false;                  // Show resource limits (--limits).
    bool showCgroupInfo = false;              // Show cgroup membership (--cgroup).
    bool showPerf = false;                    // Show performance metrics (--perf).
    static constexpr int kDefaultPerfDurationMs = 200;
    int perfDurationMs = kDefaultPerfDurationMs; // Sample duration in ms for --perf.
    static constexpr int kDefaultWatchIntervalSeconds = 2;
    std::optional<int> watchIntervalSeconds;  // --watch [SECONDS]: continuous refresh of the system command.
    std::optional<int> topCount;              // --count N: row limit for the top command.
    bool topByIo = false;                     // --io: rank the top command by disk I/O instead of CPU.
    bool topByMem = false;                    // --mem: rank the top command by resident memory.
};

std::optional<ParsedArguments> parseCommandLine(int argc, std::span<char* const> argv);

void printUsage();

#endif // CLI_ARGS_H
