// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "cli/args.h"
#include "utils/string.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <array>
#include <limits>
#include <regex>
#include <csignal>
#include <unordered_map>

namespace {
    constexpr std::array<std::string_view, 15> validColumns = {
        "pid", "ppid", "uid", "user", "name", "state", "rss", "vm",
        "threads", "cmdline", "start-time", "elapsed-time",
        "exec-path", "nice", "cwd"
    };

    bool isValidColumn(std::string_view column) {
        return std::ranges::find(validColumns, column) != validColumns.end();
    }

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
        if (lowerS == "start-time") return ProcessSortField::startTime;
        if (lowerS == "cmdline") return ProcessSortField::cmdline;
        if (lowerS == "exec-path") return ProcessSortField::executablePath;
        if (lowerS == "cwd") return ProcessSortField::cwd;
        if (lowerS == "cpu-time") return ProcessSortField::cpuTime;
        if (lowerS == "cpu-user-time") return ProcessSortField::cpuUserTime;
        if (lowerS == "cpu-kernel-time") return ProcessSortField::cpuKernelTime;
        if (lowerS == "io-read") return ProcessSortField::ioReadBytes;
        if (lowerS == "io-write") return ProcessSortField::ioWriteBytes;
        if (lowerS == "priority") return ProcessSortField::priority;
        return std::nullopt;
    }

    std::optional<int> parseIntWithinRange(std::string_view value) {
        constexpr int base10 = 10;
        const auto parsed = utils::toLong(value, base10);
        if (!parsed) {
            return std::nullopt;
        }
        if (*parsed < std::numeric_limits<int>::min() || *parsed > std::numeric_limits<int>::max()) {
            return std::nullopt;
        }
        return static_cast<int>(*parsed);
    }

    std::optional<uint16_t> parsePort(std::string_view value) {
        constexpr int base10 = 10;
        constexpr long kMaxPort = 65535;
        const auto parsed = utils::toLong(value, base10);
        if (!parsed || *parsed < 1 || *parsed > kMaxPort) {
            return std::nullopt;
        }
        return static_cast<uint16_t>(*parsed);
    }

    // Resolve a signal spec for the `signal` command: either a number (0 = an
    // existence probe, through SIGRTMAX) or a case-insensitive name with an
    // optional "SIG" prefix (e.g. TERM, SIGTERM, term). Returns nullopt when the
    // spec is neither a known name nor an in-range number.
    std::optional<int> resolveSignal(std::string_view spec) {
        if (auto num = parseIntWithinRange(spec)) {
            constexpr int kMinSignal = 0;
            constexpr int kMaxSignal = 64; // SIGRTMAX on Linux
            if (*num < kMinSignal || *num > kMaxSignal) {
                return std::nullopt;
            }
            return num;
        }
        std::string name = utils::toLower(spec);
        if (name.starts_with("sig")) {
            name = name.substr(3);
        }
        static const std::unordered_map<std::string, int> kSignalNames = {
            {"hup", SIGHUP},   {"int", SIGINT},   {"quit", SIGQUIT}, {"ill", SIGILL},
            {"trap", SIGTRAP}, {"abrt", SIGABRT}, {"bus", SIGBUS},   {"fpe", SIGFPE},
            {"kill", SIGKILL}, {"usr1", SIGUSR1}, {"segv", SIGSEGV}, {"usr2", SIGUSR2},
            {"pipe", SIGPIPE}, {"alrm", SIGALRM}, {"term", SIGTERM}, {"chld", SIGCHLD},
            {"cont", SIGCONT}, {"stop", SIGSTOP}, {"tstp", SIGTSTP}, {"ttin", SIGTTIN},
            {"ttou", SIGTTOU}, {"winch", SIGWINCH},
        };
        if (auto it = kSignalNames.find(name); it != kSignalNames.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Parse a CPU list for the `affinity` command: comma-separated indices and
    // inclusive a-b ranges (e.g. "0,2-3,5"). Returns a sorted, de-duplicated set,
    // or nullopt for an empty token, a reversed range, or an out-of-range index.
    std::optional<std::vector<int>> parseCpuList(std::string_view spec) {
        constexpr int kMinCpu = 0;
        constexpr int kMaxCpu = 4095; // generous upper bound on CPU index
        std::vector<int> cpus;
        std::size_t start = 0;
        while (true) {
            const std::size_t comma = spec.find(',', start);
            const std::string_view token =
                spec.substr(start, comma == std::string_view::npos ? std::string_view::npos
                                                                   : comma - start);
            if (token.empty()) {
                return std::nullopt;
            }
            if (const std::size_t dash = token.find('-'); dash == std::string_view::npos) {
                const auto num = parseIntWithinRange(token);
                if (!num || *num < kMinCpu || *num > kMaxCpu) {
                    return std::nullopt;
                }
                cpus.push_back(*num);
            } else {
                const auto lo = parseIntWithinRange(token.substr(0, dash));
                const auto hi = parseIntWithinRange(token.substr(dash + 1));
                if (!lo || !hi || *lo < kMinCpu || *hi > kMaxCpu || *lo > *hi) {
                    return std::nullopt;
                }
                for (int cpu = *lo; cpu <= *hi; ++cpu) {
                    cpus.push_back(cpu);
                }
            }
            if (comma == std::string_view::npos) {
                break;
            }
            start = comma + 1;
        }
        std::ranges::sort(cpus);
        const auto dup = std::ranges::unique(cpus);
        cpus.erase(dup.begin(), dup.end());
        return cpus;
    }
}

void printUsage() {
    std::cout << "Usage: processAnalyzer <command> [options]\n"
              << "Commands:\n"
              << "  list                     List all processes (default command)\n"
              << "  show --pid <pid>         Show detailed information for a specific process\n"
              << "  pid <pid>                Alias for 'show --pid <pid>'\n"
              << "  name <process_name>      Search processes by name\n"
              << "  user <username>          Search processes by user\n"
              << "  system                   Display system-wide information and statistics\n"
              << "  top                      Show processes ranked by live CPU, memory, or disk I/O\n"
              << "  signal <pid> <signal>    Send a signal to a process (name or number, e.g. TERM, KILL, 9)\n"
              << "  renice <pid> <nice>      Set a process nice value (-20..19; lower is higher priority)\n"
              << "  affinity <pid> <cpus>    Set a process CPU affinity (indices/ranges, e.g. 0,2-3)\n"
              << "\nTop command options:\n"
              << "  --count <N>              Limit the ranking to the top N processes (default 15)\n"
              << "  --io                     Rank by disk I/O instead of CPU\n"
              << "  --mem                    Rank by resident memory (no sampling delay; excludes --io)\n"
              << "  --output json            Emit the ranking as JSON (system and top accept json only)\n"
              << "  --name/--user/--state    Rank only processes matching the standard filters\n"
              << "\nOptions:\n"
              << "  -h, --help               Show this help message\n"
              << "  --watch [SECONDS]        Continuously refresh the system or top command output (default 2s)\n"
              << "  -p, --pid <pid>          Filter or show details for a specific Process ID\n"
              << "  --name <name>            Filter processes by name (contains)\n"
              << "  -u, --user <username>    Filter processes by username\n"
              << "  -s, --state <char>       Filter processes by state (e.g., 'R', 'S', 'Z')\n"
              << "  --sort-by <field>        Sort processes by a specific field (pid, ppid, uid, user, name, state,\n"
              << "                           rss, vm, threads, start-time, cmdline, exec-path, cwd,\n"
              << "                           cpu-time, cpu-user-time, cpu-kernel-time, io-read, io-write, priority)\n"
              << "  --sort-order <asc|desc>  Sort order (ascending or descending, default: asc)\n"
              << "  -b, --brief              Show brief process information (less columns)\n"
              << "  --columns <col1,col2,...> Select specific columns to display\n"
              << "                           Valid: pid, ppid, uid, user, name, state, rss, vm,\n"
              << "                           threads, cmdline, start-time, elapsed-time, exec-path, nice, cwd\n"
              << "  --no-truncate-cmdline    Do not truncate command line output\n"
              << "  -o, --output <format>    Output format (table, vertical, csv, json, ndjson, tree, default: table)\n"
              << "  --children               (With 'show' or 'pid') Show child processes\n"
              << "  --descendants            (With 'show' or 'pid') Show the full descendant subtree\n"
              << "  --open-files             (With 'show' or 'pid') Show open files for process\n"
              << "  --threads                (With 'show' or 'pid') Show threads for process\n"
              << "  --network                (With 'show' or 'pid') Show network connections for process\n"
              << "  --env, --environment     (With 'show' or 'pid') Show environment variables\n"
              << "  --maps                   (With 'show' or 'pid') Show memory maps\n"
              << "  --limits                 (With 'show' or 'pid') Show resource limits\n"
              << "  --cgroup                 (With 'show' or 'pid') Show cgroup membership\n"
              << "  --affinity               (With 'show' or 'pid') Show CPU affinity mask\n"
              << "  --ppid <ppid>            Filter processes by Parent Process ID\n"
              << "  --uid <N>                Filter processes by numeric User ID\n"
              << "  --min-rss <KB>           Filter processes with RSS >= KB\n"
              << "  --max-rss <KB>           Filter processes with RSS <= KB\n"
              << "  --min-threads <N>        Filter processes with thread count >= N\n"
              << "  --max-threads <N>        Filter processes with thread count <= N\n"
              << "  --cmdline <pattern>      Filter processes by command-line substring\n"
              << "  --exec-path <pattern>    Filter processes by executable path (contains)\n"
              << "  --name-regex <pattern>   Filter processes by name (regular expression)\n"
              << "  --cmdline-regex <pattern> Filter processes by command line (regular expression)\n"
              << "  --exec-path-regex <pattern> Filter processes by executable path (regular expression)\n"
              << "  --min-vm <KB>            Filter processes with virtual memory >= KB\n"
              << "  --max-vm <KB>            Filter processes with virtual memory <= KB\n"
              << "  --min-priority <N>       Filter processes with priority >= N\n"
              << "  --max-priority <N>       Filter processes with priority <= N\n"
              << std::endl;
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
            if (potentialCommand == "list" || potentialCommand == "show" || potentialCommand == "pid" ||
                potentialCommand == "name" || potentialCommand == "user" || potentialCommand == "system" ||
                potentialCommand == "top" || potentialCommand == "signal" ||
                potentialCommand == "renice" || potentialCommand == "affinity") {
                args.command = potentialCommand;
                cliArgs.erase(cliArgs.begin()); // Consume the command
                if (args.command == "pid") {
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'pid' command requires a PID value.\n";
                        return std::nullopt;
                    }
                    if (auto pid = parseIntWithinRange(cliArgs.front())) {
                        args.pid = pid;
                        cliArgs.erase(cliArgs.begin());
                    } else {
                        std::cerr << "Error: Invalid PID '" << cliArgs.front() << "'.\n";
                        return std::nullopt;
                    }
                } else if (args.command == "name") {
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'name' command requires a process name.\n";
                        return std::nullopt;
                    }
                    args.name = cliArgs.front();
                    cliArgs.erase(cliArgs.begin());
                } else if (args.command == "user") {
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'user' command requires a username.\n";
                        return std::nullopt;
                    }
                    args.user = cliArgs.front();
                    cliArgs.erase(cliArgs.begin());
                } else if (args.command == "signal") {
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'signal' command requires a PID value.\n";
                        return std::nullopt;
                    }
                    if (auto pid = parseIntWithinRange(cliArgs.front())) {
                        args.pid = pid;
                        cliArgs.erase(cliArgs.begin());
                    } else {
                        std::cerr << "Error: Invalid PID '" << cliArgs.front() << "'.\n";
                        return std::nullopt;
                    }
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'signal' command requires a signal name or number.\n";
                        return std::nullopt;
                    }
                    if (auto resolved = resolveSignal(cliArgs.front())) {
                        args.signalNumber = resolved;
                        cliArgs.erase(cliArgs.begin());
                    } else {
                        std::cerr << "Error: Invalid signal '" << cliArgs.front() << "'.\n";
                        return std::nullopt;
                    }
                } else if (args.command == "renice") {
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'renice' command requires a PID value.\n";
                        return std::nullopt;
                    }
                    if (auto pid = parseIntWithinRange(cliArgs.front())) {
                        args.pid = pid;
                        cliArgs.erase(cliArgs.begin());
                    } else {
                        std::cerr << "Error: Invalid PID '" << cliArgs.front() << "'.\n";
                        return std::nullopt;
                    }
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'renice' command requires a nice value (-20..19).\n";
                        return std::nullopt;
                    }
                    constexpr int kMinNice = -20;
                    constexpr int kMaxNice = 19;
                    auto nice = parseIntWithinRange(cliArgs.front());
                    if (!nice || *nice < kMinNice || *nice > kMaxNice) {
                        std::cerr << "Error: Invalid nice value '" << cliArgs.front()
                                  << "' (expected -20..19).\n";
                        return std::nullopt;
                    }
                    args.niceValue = nice;
                    cliArgs.erase(cliArgs.begin());
                } else if (args.command == "affinity") {
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'affinity' command requires a PID value.\n";
                        return std::nullopt;
                    }
                    if (auto pid = parseIntWithinRange(cliArgs.front())) {
                        args.pid = pid;
                        cliArgs.erase(cliArgs.begin());
                    } else {
                        std::cerr << "Error: Invalid PID '" << cliArgs.front() << "'.\n";
                        return std::nullopt;
                    }
                    if (cliArgs.empty()) {
                        std::cerr << "Error: 'affinity' command requires a CPU list (e.g. 0,2-3).\n";
                        return std::nullopt;
                    }
                    if (auto cpus = parseCpuList(cliArgs.front())) {
                        args.affinityCpus = std::move(cpus);
                        cliArgs.erase(cliArgs.begin());
                    } else {
                        std::cerr << "Error: Invalid CPU list '" << cliArgs.front()
                                  << "' (expected indices/ranges like 0,2-3).\n";
                        return std::nullopt;
                    }
                }
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
        }
        if (arg == "--pid" || arg == "-p") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --pid requires an argument.\n";
                return std::nullopt;
            }
            if (auto pid = parseIntWithinRange(cliArgs[++i])) {
                args.pid = pid;
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
            for (auto& column : args.selectedColumns) {
                column = utils::toLower(utils::trim(column));
                if (!isValidColumn(column)) {
                    std::cerr << "Error: Invalid column '" << column << "'.\n";
                    return std::nullopt;
                }
            }
        } else if (arg == "--no-truncate-cmdline") {
            args.noTruncateCmdline = true;
        } else if (arg == "--output" || arg == "-o") { // Renamed from --format to --output
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --output requires an argument.\n";
                return std::nullopt;
            }
            std::string format = utils::toLower(cliArgs[++i]);
            if (format == "csv" || format == "json" || format == "table" ||
                format == "vertical" || format == "tree" || format == "ndjson") {
                args.outputFormat = format;
            } else {
                std::cerr << "Error: Invalid output format '" << cliArgs[i] << "'. Use 'csv', 'json', 'ndjson', 'table', 'vertical', or 'tree'.\n";
                return std::nullopt;
            }
        } else if (arg == "--children") {
            args.showChildren = true;
        } else if (arg == "--descendants") {
            args.showDescendants = true;
        } else if (arg == "--threads") {
            args.showThreads = true;
        } else if (arg == "--open-files") {
            args.showOpenFiles = true;
        } else if (arg == "--ppid") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --ppid requires an argument.\n";
                return std::nullopt;
            }
            if (auto ppid = parseIntWithinRange(cliArgs[++i])) {
                args.ppidFilter = ppid;
            } else {
                std::cerr << "Error: Invalid PPID '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--network") {
            // If the next token is a valid port number, treat as list-filter.
            if (i + 1 < cliArgs.size()) {
                if (auto port = parsePort(cliArgs[i + 1])) {
                    args.networkPortFilter = port;
                    ++i;
                } else {
                    args.showNetworkConnections = true;
                }
            } else {
                args.showNetworkConnections = true;
            }
        } else if (arg == "--env" || arg == "--environment") {
            args.showEnv = true;
        } else if (arg == "--maps") {
            args.showMemoryMaps = true;
        } else if (arg == "--limits") {
            args.showLimits = true;
        } else if (arg == "--cgroup") {
            args.showCgroupInfo = true;
        } else if (arg == "--affinity") {
            args.showAffinity = true;
        } else if (arg == "--perf") {
            args.showPerf = true;
            if (i + 1 < cliArgs.size()) {
                if (auto dur = parseIntWithinRange(cliArgs[i + 1]); dur && *dur > 0) {
                    args.perfDurationMs = static_cast<int>(*dur);
                    ++i;
                }
            }
        } else if (arg == "--watch") {
            args.watchIntervalSeconds = ParsedArguments::kDefaultWatchIntervalSeconds;
            if (i + 1 < cliArgs.size()) {
                if (auto secs = parseIntWithinRange(cliArgs[i + 1]); secs && *secs > 0) {
                    args.watchIntervalSeconds = static_cast<int>(*secs);
                    ++i;
                }
            }
        } else if (arg == "--count") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --count requires a positive integer argument.\n";
                return std::nullopt;
            }
            if (auto count = parseIntWithinRange(cliArgs[++i]); count && *count > 0) {
                args.topCount = static_cast<int>(*count);
            } else {
                std::cerr << "Error: Invalid value for --count '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--io") {
            args.topByIo = true;
        } else if (arg == "--mem") {
            args.topByMem = true;
        } else if (arg == "--uid") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --uid requires an argument.\n";
                return std::nullopt;
            }
            if (auto uid = parseIntWithinRange(cliArgs[++i])) {
                if (*uid < 0) {
                    std::cerr << "Error: --uid requires a non-negative integer.\n";
                    return std::nullopt;
                }
                args.uidFilter = uid;
            } else {
                std::cerr << "Error: Invalid UID '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--min-rss") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --min-rss requires an argument (KB).\n";
                return std::nullopt;
            }
            if (auto val = utils::toLong(cliArgs[++i])) {
                args.minRssKb = static_cast<long long>(*val);
            } else {
                std::cerr << "Error: Invalid value for --min-rss '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--max-rss") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --max-rss requires an argument (KB).\n";
                return std::nullopt;
            }
            if (auto val = utils::toLong(cliArgs[++i])) {
                args.maxRssKb = static_cast<long long>(*val);
            } else {
                std::cerr << "Error: Invalid value for --max-rss '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--min-threads") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --min-threads requires an argument.\n";
                return std::nullopt;
            }
            if (auto val = utils::toLong(cliArgs[++i])) {
                args.minThreads = *val;
            } else {
                std::cerr << "Error: Invalid value for --min-threads '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--max-threads") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --max-threads requires an argument.\n";
                return std::nullopt;
            }
            if (auto val = utils::toLong(cliArgs[++i])) {
                args.maxThreads = *val;
            } else {
                std::cerr << "Error: Invalid value for --max-threads '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--cmdline") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --cmdline requires an argument.\n";
                return std::nullopt;
            }
            args.cmdlineFilter = std::string(cliArgs[++i]);
        } else if (arg == "--exec-path") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --exec-path requires an argument.\n";
                return std::nullopt;
            }
            args.executablePathFilter = std::string(cliArgs[++i]);
        } else if (arg == "--name-regex" || arg == "--cmdline-regex" ||
                   arg == "--exec-path-regex") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: " << arg << " requires a pattern argument.\n";
                return std::nullopt;
            }
            const std::string& pattern = cliArgs[++i];
            try {
                // Validate the pattern up front so an invalid regex fails parsing
                // rather than surfacing later when the filter is built.
                std::regex compiled(pattern);
                (void)compiled;
            } catch (const std::regex_error&) {
                std::cerr << "Error: invalid regex for " << arg << " '" << pattern << "'.\n";
                return std::nullopt;
            }
            if (arg == "--name-regex") {
                args.nameRegexPattern = pattern;
            } else if (arg == "--cmdline-regex") {
                args.cmdlineRegexPattern = pattern;
            } else {
                args.executablePathRegexPattern = pattern;
            }
        } else if (arg == "--min-vm") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --min-vm requires an argument (KB).\n";
                return std::nullopt;
            }
            if (auto val = utils::toLong(cliArgs[++i])) {
                args.minVmKb = static_cast<long long>(*val);
            } else {
                std::cerr << "Error: Invalid value for --min-vm '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--max-vm") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --max-vm requires an argument (KB).\n";
                return std::nullopt;
            }
            if (auto val = utils::toLong(cliArgs[++i])) {
                args.maxVmKb = static_cast<long long>(*val);
            } else {
                std::cerr << "Error: Invalid value for --max-vm '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--min-priority") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --min-priority requires an argument.\n";
                return std::nullopt;
            }
            args.minPriority = parseIntWithinRange(cliArgs[++i]);
            if (!args.minPriority) {
                std::cerr << "Error: Invalid value for --min-priority '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
        } else if (arg == "--max-priority") {
            if (i + 1 >= cliArgs.size()) {
                std::cerr << "Error: --max-priority requires an argument.\n";
                return std::nullopt;
            }
            args.maxPriority = parseIntWithinRange(cliArgs[++i]);
            if (!args.maxPriority) {
                std::cerr << "Error: Invalid value for --max-priority '" << cliArgs[i] << "'.\n";
                return std::nullopt;
            }
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
    if (args.command == "pid" && !args.pid.has_value()) {
        std::cerr << "Error: 'pid' command requires a PID value.\n";
        return std::nullopt;
    }

    // Inspection flags are only valid with 'show' or 'pid' commands.
    if ((args.showChildren || args.showDescendants || args.showOpenFiles || args.showNetworkConnections ||
         args.showThreads || args.showEnv || args.showMemoryMaps ||
         args.showLimits || args.showCgroupInfo || args.showAffinity || args.showPerf) &&
        args.command != "show" && args.command != "pid") {
        std::cerr << "Error: --children, --descendants, --open-files, --threads, --network, --env, --maps, --limits, --cgroup, --affinity, and --perf are only valid with 'show' or 'pid' commands.\n";
        return std::nullopt;
    }

    // --network <port> (port filter) is only valid with 'list' command.
    if (args.networkPortFilter.has_value() &&
        args.command != "list" && args.command != "name" && args.command != "user") {
        std::cerr << "Error: --network <port> filter is only valid with the 'list' command.\n";
        return std::nullopt;
    }

    // --ppid cannot be used with single-PID commands.
    if (args.ppidFilter.has_value() && (args.command == "show" || args.command == "pid")) {
        std::cerr << "Error: --ppid cannot be used with 'show' or 'pid' command.\n";
        return std::nullopt;
    }

    // --watch refreshes the system and top commands.
    if (args.watchIntervalSeconds.has_value() &&
        args.command != "system" && args.command != "top") {
        std::cerr << "Error: --watch is only valid with the 'system' and 'top' commands.\n";
        return std::nullopt;
    }

    // --count, --io, and --mem only apply to the top command.
    if ((args.topCount.has_value() || args.topByIo || args.topByMem) && args.command != "top") {
        std::cerr << "Error: --count, --io, and --mem are only valid with the 'top' command.\n";
        return std::nullopt;
    }
    // --io and --mem select mutually exclusive ranking modes.
    if (args.topByIo && args.topByMem) {
        std::cerr << "Error: --io and --mem cannot be combined.\n";
        return std::nullopt;
    }

    // The system and top commands render only their own human report or a JSON
    // payload; csv/table/vertical are accepted by the generic --output parser but
    // never honored here, so reject them instead of silently discarding the flag.
    if ((args.command == "system" || args.command == "top") &&
        args.outputFormat.has_value() && *args.outputFormat != "json") {
        std::cerr << "Error: the '" << args.command
                  << "' command supports '--output json' only.\n";
        return std::nullopt;
    }

    return args;
}

bool hasStaticProcessFilter(const ParsedArguments& args) {
    // Must list every args field that populates a ProcessFilter static criterion in
    // main.cpp (name/cmdline substring + regex, user, state, uid, ppid, and the
    // thread/memory/priority ranges). The name and cmdline regex patterns were
    // historically missing here, so `top --name-regex`/`--cmdline-regex` set a filter
    // that was never applied.
    return args.name || args.nameRegexPattern || args.cmdlineFilter ||
           args.cmdlineRegexPattern || args.executablePathFilter ||
           args.executablePathRegexPattern || args.user || args.stateFilter ||
           args.uidFilter || args.ppidFilter || args.minRssKb || args.maxRssKb ||
           args.minVmKb || args.maxVmKb || args.minThreads || args.maxThreads ||
           args.minPriority || args.maxPriority;
}
