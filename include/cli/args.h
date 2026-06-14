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
    bool showDescendants = false;             // Show the full descendant subtree (--descendants).
    bool showOpenFiles = false;
    bool showThreads = false;
    bool showHelp = false;
    std::optional<int> ppidFilter;            // Stores the PPID value for the --ppid filter.
    bool showNetworkConnections = false;      // Flag to indicate if --network option was used.
    std::optional<uint16_t> networkPortFilter; // Local port for --network <port> list filter.
    std::optional<uint16_t> networkRemotePort;             // Remote port (--remote-port).
    std::optional<std::string> networkRemoteAddrFilter;       // Remote address substring (--remote-addr).
    std::optional<std::string> networkRemoteAddrRegexPattern; // Remote address regex (--remote-addr-regex).
    std::optional<std::string> networkProtocol;            // Connection protocol, e.g. TCP/UDP (--net-protocol).
    std::optional<std::string> networkState;               // Connection state, e.g. LISTEN (--net-state).
    std::optional<long long> minRssKb;        // Minimum resident memory in KB (--min-rss).
    std::optional<long long> maxRssKb;        // Maximum resident memory in KB (--max-rss).
    std::optional<long> minThreads;           // Minimum thread count (--min-threads).
    std::optional<long> maxThreads;           // Maximum thread count (--max-threads).
    std::optional<int> uidFilter;             // Filter by numeric UID (--uid).
    std::optional<std::string> cmdlineFilter; // Filter by cmdline substring (--cmdline).
    std::optional<std::string> nameRegexPattern;    // Filter by name regex (--name-regex).
    std::optional<std::string> cmdlineRegexPattern; // Filter by cmdline regex (--cmdline-regex).
    std::optional<std::string> executablePathFilter;       // Filter by executable path substring (--exec-path).
    std::optional<std::string> executablePathRegexPattern; // Filter by executable path regex (--exec-path-regex).
    std::optional<long long> minVmKb;         // Minimum virtual memory in KB (--min-vm).
    std::optional<long long> maxVmKb;         // Maximum virtual memory in KB (--max-vm).
    std::optional<int> minPriority;           // Minimum process priority (--min-priority).
    std::optional<int> maxPriority;           // Maximum process priority (--max-priority).
    bool showEnv = false;                     // Show environment variables (--env).
    bool showMemoryMaps = false;              // Show memory maps (--maps).
    bool showLimits = false;                  // Show resource limits (--limits).
    bool showCgroupInfo = false;              // Show cgroup membership (--cgroup).
    bool showAffinity = false;                // Show CPU affinity mask (--affinity).
    bool showPerf = false;                    // Show performance metrics (--perf).
    static constexpr int kDefaultPerfDurationMs = 200;
    int perfDurationMs = kDefaultPerfDurationMs; // Sample duration in ms for --perf.
    static constexpr int kDefaultWatchIntervalSeconds = 2;
    std::optional<int> watchIntervalSeconds;  // --watch [SECONDS]: continuous refresh of the system command.
    std::optional<int> topCount;              // --count N: row limit for the top command.
    bool topByIo = false;                     // --io: rank the top command by disk I/O instead of CPU.
    bool topByMem = false;                    // --mem: rank the top command by resident memory.
    std::optional<int> signalNumber;          // Resolved signal number for the `signal` command.
    std::optional<int> niceValue;             // Nice value (-20..19) for the `renice` command.
    std::optional<std::vector<int>> affinityCpus; // Sorted, unique CPU set for the `affinity` command.
};

std::optional<ParsedArguments> parseCommandLine(int argc, std::span<char* const> argv);

void printUsage();

// True when any static process-filter criterion is set (name/substring, name or
// cmdline regex, user, state, uid, ppid, thread/memory/priority ranges). The `top`
// command uses this to decide whether to restrict its ranking to matching processes
// (mirroring `list`). Centralizing the predicate keeps it in lockstep with the
// ProcessFilter populated in main.cpp, so a newly added filter can never be silently
// omitted from the gate (as the name/cmdline regex filters once were).
[[nodiscard]] bool hasStaticProcessFilter(const ParsedArguments& args);

#endif // CLI_ARGS_H
