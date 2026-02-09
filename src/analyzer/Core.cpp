// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <cstdint> // For uint32_t
#include <array>   // For std::array
#include <limits>  // For std::numeric_limits
#include <string>  // For std::to_string

#include "analyzer/Core.h"
#include "utils/Core.h"

#include <filesystem>
#include <sstream>
#include <algorithm>
#include <ranges> // For std::ranges::copy_if
#include <pwd.h>        // For getpwuid
#include <sys/types.h>
#include <sys/stat.h>   // For fstatat, S_ISREG, etc.
// #include <string_view> // For std::string_view::starts_with - No longer needed after procPath type change
#include <chrono>
#include <thread>



#include <unistd.h>     // For sysconf, gethostname
#include <csignal>     // For kill
#include <sys/statvfs.h> // for statvfs
#include <netinet/in.h> // for INET6_ADDRSTRLEN
#include <arpa/inet.h>  // for inet_ntop
#include <fcntl.h>      // For fstatat
#include <dirent.h>     // For dirfd, opendir
#include <regex>

#include <set>
#include <sys/resource.h> // For setpriority
#include <sched.h>        // For sched_setaffinity


namespace {
    namespace fs = std::filesystem;
    // Number of fields to skip after ppid (4th field) and before utime (14th field)
    // Fields skipped: pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt
    // Count: 9 fields (from 5th to 13th, 1-based indexing)
    constexpr int statFieldsToSkipBeforeUtime = 9;

    // Number of fields to skip after nice (19th field) and before starttime (22nd field)
    // Fields skipped: num_threads, itrealvalue
    // Count: 3 fields (from 19th to 21st, 1-based indexing)
    constexpr int statFieldsToSkipBeforeStarttime = 3;
    constexpr int statFieldsToSkipBeforeThreadUtime = 10;
    constexpr double msInSecond = 1000.0;
    constexpr long defaultSystemClockTicks = 100;


    utils::Result<long long> getTotalSystemCpuTimeTicks(const ::std::filesystem::path& procPath) {
        // Construct ProcessAnalyzer with the filesystem::path
        auto stats = ProcessAnalyzer(procPath).getSystemCpuStats();
        if(stats) {
            unsigned long long totalTicks = stats->user + stats->nice + stats->system + stats->idle + 
                                            stats->iowait + stats->irq + stats->softirq + stats->steal;
            return static_cast<long long>(totalTicks);
        }
        return ::std::unexpected(stats.error());
    }




    
    // Internal struct to temporarily hold NetworkConnection data along with its inode
    // for filtering purposes before converting to the public NetworkConnection struct.
    struct InternalNetworkConnection {
        NetworkConnection baseConn;
        int inode;
    };

    enum class TcpState : std::uint8_t {
        established = 1,
        synSent,
        synRecv,
        finWait1,
        finWait2,
        timeWait,
        close,
        closeWait,
        lastAck,
        listen,
        closing,
        unknown
    };
    constexpr int ipv6LineDummyCount = 5;

    // IPv6 parsing constants
    constexpr int ipv6HexBase = 16;
    constexpr size_t ipv6AddrHexLength = 32;
    constexpr size_t ipv6AddrPartHexLength = 8;
    constexpr size_t ipv6AddrPartOffset0 = 0;
    constexpr size_t ipv6AddrPartOffset1 = 8;
    constexpr size_t ipv6AddrPartOffset2 = 16;
    constexpr size_t ipv6AddrPartOffset3 = 24;

    ::std::vector<InternalNetworkConnection> parseNetFileHelper(const ::std::filesystem::path& filePath, ::std::string_view protocolPrefix) {
        ::std::vector<InternalNetworkConnection> internalConnections;
        auto content = utils::readTextFile(filePath.string()); // Read file content
        if (!content) {
            return internalConnections;
        }

        ::std::stringstream ss(*content);
        ::std::string line;
        ::std::getline(ss, line); // Skip header

        while (::std::getline(ss, line)) {
            ::std::stringstream lineSs(line);
            ::std::string sl, localAddrStr, remoteAddrStr, stStr, txRxQueue, trTmWhen, retrnsmt, uid, timeout, inodeStr;
            
            if (!(lineSs >> sl >> localAddrStr >> remoteAddrStr >> stStr >> txRxQueue >> trTmWhen >> retrnsmt >> uid >> timeout >> inodeStr)) {
                continue;
            }

            InternalNetworkConnection internalConn;
            internalConn.baseConn.protocol = protocolPrefix;
            internalConn.inode = 0;
            if (utils::isInteger(inodeStr)) {
                internalConn.inode = std::stoi(inodeStr);
            }

            // Parse State
            int stateInt = 0;
            try {
                stateInt = std::stoi(stStr, nullptr, 16);
            } catch (...) {}
            
            switch (static_cast<TcpState>(stateInt)) {
                case TcpState::established: internalConn.baseConn.state = "ESTABLISHED"; break;
                case TcpState::synSent: internalConn.baseConn.state = "SYN_SENT"; break;
                case TcpState::synRecv: internalConn.baseConn.state = "SYN_RECV"; break;
                case TcpState::finWait1: internalConn.baseConn.state = "FIN_WAIT1"; break;
                case TcpState::finWait2: internalConn.baseConn.state = "FIN_WAIT2"; break;
                case TcpState::timeWait: internalConn.baseConn.state = "TIME_WAIT"; break;
                case TcpState::close: internalConn.baseConn.state = "CLOSE"; break;
                case TcpState::closeWait: internalConn.baseConn.state = "CLOSE_WAIT"; break;
                case TcpState::lastAck: internalConn.baseConn.state = "LAST_ACK"; break;
                case TcpState::listen: internalConn.baseConn.state = "LISTEN"; break;
                case TcpState::closing: internalConn.baseConn.state = "CLOSING"; break;
                default: internalConn.baseConn.state = "UNKNOWN"; break;
            }

            auto parseIpPort = [](const std::string& addrStr) -> std::string {
                size_t colonPos = addrStr.find(':');
                if (colonPos == std::string::npos) return addrStr;
                
                std::string ipHex = addrStr.substr(0, colonPos);
                std::string portHex = addrStr.substr(colonPos + 1);
                
                long port = 0;
                try {
                    port = std::stol(portHex, nullptr, 16);
                } catch (...) {}
                
                if (ipHex.length() == 8) { // IPv4
                    unsigned int ipValue;
                    try {
                        ipValue = std::stoul(ipHex, nullptr, 16);
                    } catch(...) { return addrStr; }
                    const unsigned octet1 = ipValue & 0xFFU;
                    const unsigned octet2 = (ipValue >> 8U) & 0xFFU;
                    const unsigned octet3 = (ipValue >> 16U) & 0xFFU;
                    const unsigned octet4 = (ipValue >> 24U) & 0xFFU;
                    return std::to_string(octet1) + "." + std::to_string(octet2) + "." +
                           std::to_string(octet3) + "." + std::to_string(octet4) + ":" +
                           std::to_string(port);
                } else if (ipHex.length() == 32) { // IPv6
                    // IPv6 is 4 32-bit integers.
                    struct in6_addr in6;
                    for(int i=0; i<4; ++i) {
                        try {
                            in6.s6_addr32[i] = std::stoul(ipHex.substr(i*8, 8), nullptr, 16);
                        } catch(...) {}
                    }
                    char buf[INET6_ADDRSTRLEN];
                    if (inet_ntop(AF_INET6, &in6, buf, sizeof(buf))) {
                        return std::string(buf) + ":" + std::to_string(port);
                    }
                }
                return addrStr;
            };

            internalConn.baseConn.localAddress = parseIpPort(localAddrStr);
            internalConn.baseConn.remoteAddress = (remoteAddrStr == "00000000:0000" || remoteAddrStr == "00000000000000000000000000000000:0000") ? "*" : parseIpPort(remoteAddrStr);

            internalConnections.push_back(internalConn);
        }
        return internalConnections;
    }

    // Helper function to apply process filters
    bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter) {
        if (filter.nameContains && process.name.find(*filter.nameContains) == ::std::string::npos) return false;
        if (filter.nameRegex && !::std::regex_search(process.name, *filter.nameRegex)) return false;
        
        if (filter.userFilter && process.username != *filter.userFilter) return false;
        if (filter.stateFilter && process.state.empty()) return false; // Added check for empty state
        if (filter.stateFilter && process.state[0] != *filter.stateFilter) return false;
        
        if (filter.minThreads && process.threadCount < *filter.minThreads) return false;
        if (filter.maxThreads && process.threadCount > *filter.maxThreads) return false;
        
        if (filter.minResidentMemoryKB && process.residentMemory < *filter.minResidentMemoryKB) return false;
        if (filter.maxResidentMemoryKB && process.residentMemory > *filter.maxResidentMemoryKB) return false;
        if (filter.minVirtualMemoryKB && process.virtualMemory < *filter.minVirtualMemoryKB) return false;
        if (filter.maxVirtualMemoryKB && process.virtualMemory > *filter.maxVirtualMemoryKB) return false;
        
        if (filter.cmdlineContains && process.cmdline.find(*filter.cmdlineContains) == ::std::string::npos) return false;
        if (filter.cmdlineRegex && !::std::regex_search(process.cmdline, *filter.cmdlineRegex)) return false;
        
        if (filter.executablePathContains && process.executablePath.find(*filter.executablePathContains) == ::std::string::npos) return false;
        if (filter.executablePathRegex && !::std::regex_search(process.executablePath, *filter.executablePathRegex)) return false;
        
        if (filter.uidFilter && process.uid != *filter.uidFilter) return false;
        if (filter.minPriority && process.priority < *filter.minPriority) return false;
        if (filter.maxPriority && process.priority > *filter.maxPriority) return false;
        if (filter.ppidFilter && process.ppid != *filter.ppidFilter) return false;

        if (filter.minCpuUsage && process.cpuUsage < *filter.minCpuUsage) return false;
        if (filter.maxCpuUsage && process.cpuUsage > *filter.maxCpuUsage) return false;
        if (filter.minMemoryPercentage && process.memoryPercentage < *filter.minMemoryPercentage) return false;
        if (filter.maxMemoryPercentage && process.memoryPercentage > *filter.maxMemoryPercentage) return false;

        if (filter.customPredicate && !(*filter.customPredicate)(process)) return false;

        return true;
    }


    utils::Result<void> checkPidPathExistsAndPermissions(const ::std::filesystem::path& procPath, pid_t pid) {
        ::std::filesystem::path pidPath = procPath / ::std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
        }
        // Check if we can list the directory, which usually implies read permissions.
        // If not, we assume permission denied for the process directory.
        try {
            // Attempt to create a directory_iterator. If it throws, it's likely a permission issue.
            fs::directory_iterator test_iter(pidPath);
        } catch (const fs::filesystem_error& e) {
            return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        }
        return {};
    }

    // Helper function to read environment variables for a process
    utils::Result<::std::vector<::std::string>> readProcessEnvironmentVars(const ::std::filesystem::path& procPath, pid_t pid) {
        ::std::vector<::std::string> env;
        ::std::filesystem::path environPath = procPath / ::std::to_string(pid) / "environ";

        auto environContentOpt = utils::readTextFile(environPath.string());
        if (!environContentOpt) {
            auto check = checkPidPathExistsAndPermissions(procPath, pid);
            if (!check) {
                return ::std::unexpected(check.error());
            }
            // Process exists but environ file could not be read (e.g. permission denied on file, or file missing)
            // Return empty environment
            return env;
        }

        ::std::string_view content = *environContentOpt;
        size_t start = 0;
        while(start < content.size()) {
            size_t end = content.find('\0', start);
            if (end == ::std::string_view::npos) break;
            env.emplace_back(content.substr(start, end - start));
            start = end + 1;
        }
        return env;
    }

    constexpr size_t pwBufSize = 1024; // Define buffer size for getpwuid_r

    // Generic lambda for sorting processes to reduce code duplication
    auto processSortPredicate = [](const ProcessInfo& a, const ProcessInfo& b, ProcessSortField sortBy, SortOrder sortOrder) {
        auto keyLess = [&](const ProcessInfo& lhs, const ProcessInfo& rhs) {
            switch (sortBy) {
                case ProcessSortField::pid: return lhs.pid < rhs.pid;
                case ProcessSortField::ppid: return lhs.ppid < rhs.ppid;
                case ProcessSortField::uid: return lhs.uid < rhs.uid;
                case ProcessSortField::user: return lhs.username < rhs.username;
                case ProcessSortField::name: return lhs.name < rhs.name;
                case ProcessSortField::state: return lhs.state < rhs.state;
                case ProcessSortField::rss: return lhs.residentMemory < rhs.residentMemory;
                case ProcessSortField::vmsize: return lhs.virtualMemory < rhs.virtualMemory;
                case ProcessSortField::threads: return lhs.threadCount < rhs.threadCount;
                case ProcessSortField::startTime: return lhs.startTimeTicks < rhs.startTimeTicks;
                case ProcessSortField::executablePath: return lhs.executablePath < rhs.executablePath;
                case ProcessSortField::cmdline: return lhs.cmdline < rhs.cmdline;
                case ProcessSortField::cpuTime:
                    return (lhs.cpuUserTimeTicks + lhs.cpuKernelTimeTicks) < (rhs.cpuUserTimeTicks + rhs.cpuKernelTimeTicks);
                case ProcessSortField::cwd: return lhs.currentWorkingDirectory < rhs.currentWorkingDirectory;
                case ProcessSortField::cpuUserTime: return lhs.cpuUserTimeTicks < rhs.cpuUserTimeTicks;
                case ProcessSortField::cpuKernelTime: return lhs.cpuKernelTimeTicks < rhs.cpuKernelTimeTicks;
                case ProcessSortField::ioReadBytes: return lhs.ioReadBytes < rhs.ioReadBytes;
                case ProcessSortField::ioWriteBytes: return lhs.ioWriteBytes < rhs.ioWriteBytes;
                case ProcessSortField::priority: return lhs.priority < rhs.priority;
                case ProcessSortField::cpuUsage: return lhs.cpuUsage < rhs.cpuUsage;
                case ProcessSortField::memoryPercentage: return lhs.memoryPercentage < rhs.memoryPercentage;
                default: return lhs.pid < rhs.pid;
            }
        };

        const bool less = keyLess(a, b);
        const bool greater = keyLess(b, a);
        if (sortOrder == SortOrder::asc) {
            if (less) return true;
            if (greater) return false;
        } else {
            if (greater) return true;
            if (less) return false;
        }
        return a.pid < b.pid;
    };
} // Anonymous namespace ends


ProcessAnalyzer::ProcessAnalyzer(std::filesystem::path procPath = "/proc") : procPath(std::move(procPath)) {}

utils::Result<long long> ProcessAnalyzer::getSystemBootTimeUnix(const std::filesystem::path& procPath) {
    std::filesystem::path uptimePath = procPath / "uptime";
    auto contentOpt = utils::readTextFile(uptimePath.string());
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    std::stringstream ss(*contentOpt);
    double uptimeSeconds;
    ss >> uptimeSeconds;

    if (ss.fail()) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    // Calculate boot time in Unix epoch seconds
    auto now = std::chrono::system_clock::now();
    long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    long long bootTimeUnix = currentTimeUnix - static_cast<long long>(uptimeSeconds);

    return bootTimeUnix;
}


utils::Result<::std::vector<int>> ProcessAnalyzer::getPids() const {
    ::std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                ::std::string filename = entry.path().filename().string();
                if (utils::isInteger(filename)) {
                    pids.push_back(::std::stoi(filename));
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return pids;
}

utils::Result<ProcessInfo> ProcessAnalyzer::getProcessDetails(pid_t pid) const {
    ProcessInfo info;
    info.pid = pid;

    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return ::std::unexpected(check.error());
    }
    ::std::filesystem::path pidPath = procPath / ::std::to_string(pid);

    ::std::filesystem::path statusPath = pidPath / "status";
    if (auto statusContentOpt = utils::readTextFile(statusPath.string())) {
        ::std::vector<::std::string> lines = utils::split(*statusContentOpt, '\n');
        for (const auto& line : lines) {
            auto colonPos = line.find(':');
            if (colonPos == std::string::npos) continue;

            std::string key = utils::trim(line.substr(0, colonPos));
            std::string value = utils::trim(line.substr(colonPos + 1));
            // Replace tabs with spaces to handle tab-separated values (common in /proc status)
            ::std::replace(value.begin(), value.end(), '\t', ' ');

            if (key == "Name") {
                info.name = value;
            } else if (key == "State") {
                info.state = value;
            } else if (key == "VmSize") {
                auto parts = utils::split(value, ' ', true);
                if (!parts.empty()) utils::tryParse(parts[0], info.virtualMemory);
            } else if (key == "VmRSS") {
                auto parts = utils::split(value, ' ', true);
                if (!parts.empty()) utils::tryParse(parts[0], info.residentMemory);
            } else if (key == "PPid") {
                utils::tryParse(value, info.ppid);
            } else if (key == "Uid") {
                // Uid line can have multiple values, we only need the first one (real UID)
                auto firstField = utils::split(value, ' ', true);
                if (!firstField.empty()) {
                    utils::tryParse(firstField[0], info.uid);
                }
            } else if (key == "Threads") {
                utils::tryParse(value, info.threadCount);
            }
        }
    } else {
        // If status file can't be read, it's a critical error.
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::filesystem::path statPath = pidPath / "stat";
    if (auto statContentOpt = utils::readTextFile(statPath.string())) {
        ::std::string content = *statContentOpt;
        size_t lastRParen = content.rfind(')');
        
        if (lastRParen != ::std::string::npos) {
            auto firstParen = content.find('(');
            auto lastParen = content.rfind(')');

            if (firstParen != std::string::npos && lastParen != std::string::npos && lastParen > firstParen) {
                // The fields after the command name.
                std::string fields_part = utils::trim(content.substr(lastParen + 1));
                std::vector<std::string> stat_fields = utils::split(fields_part, ' ', true); // Skip empty parts

                // stat_fields indices (0-based from state):
                // 1: ppid
                // 11: utime
                // 12: stime
                // 16: nice
                // 19: starttime

                if (stat_fields.size() > 1) { // ppid
                    utils::tryParse(stat_fields[1], info.ppid);
                }
                
                unsigned long utime = 0, stime = 0;
                if (stat_fields.size() > 12) {
                    utils::tryParse(stat_fields[11], utime);
                    utils::tryParse(stat_fields[12], stime);
                    info.cpuUserTimeTicks = utime;
                    info.cpuKernelTimeTicks = stime;
                }

                if (stat_fields.size() > 16) { // nice
                    long nice_val = 0;
                    utils::tryParse(stat_fields[16], nice_val);
                    info.priority = static_cast<int>(nice_val);
                }

                if (stat_fields.size() > 19) { // starttime
                    long long starttime_val = 0;
                    utils::tryParse(stat_fields[19], starttime_val);
                    info.startTimeTicks = starttime_val;
                }
            }
        }
    }

    // Call getSystemBootTimeUnix which now returns utils::Result
    auto systemBootTimeUnixResult = getSystemBootTimeUnix(procPath);
    if (!systemBootTimeUnixResult) {
        return ::std::unexpected(systemBootTimeUnixResult.error());
    }
    long long systemBootTimeUnix = *systemBootTimeUnixResult;

    if (systemBootTimeUnix != 0) { // Check for valid boot time
        long systemClockTicks = getSystemClockTicksPerSecond().value_or(defaultSystemClockTicks);
        if (systemClockTicks > 0) {
            long long processStartTimeSec = info.startTimeTicks / systemClockTicks;
            info.startTimeUnix = systemBootTimeUnix + processStartTimeSec;
            
            auto now = ::std::chrono::system_clock::now();
            long long currentTimeUnix = ::std::chrono::duration_cast<::std::chrono::seconds>(now.time_since_epoch()).count();
            long long elapsedSeconds = currentTimeUnix - info.startTimeUnix;
            info.elapsedTime = utils::formatElapsedTime(elapsedSeconds);
        }
    }

    ::std::filesystem::path ioPath = pidPath / "io";
    if (auto ioContentOpt = utils::readTextFile(ioPath.string())) {
        ::std::vector<::std::string> lines = utils::split(*ioContentOpt, '\n');
        for (const auto& line : lines) {
            if (line.starts_with("rchar:")) {
                ::std::stringstream(line.substr(line.find(':') + 1)) >> info.ioReadBytes;
            } else if (line.starts_with("wchar:")) {
                ::std::stringstream(line.substr(line.find(':') + 1)) >> info.ioWriteBytes;
            }
        }
    }

    // Use getpwuid_r for thread-safe username retrieval
    ::std::array<char, pwBufSize> pwBuf;
    struct passwd pwd;
    struct passwd *result = nullptr;

    if (getpwuid_r(info.uid, &pwd, pwBuf.data(), pwBuf.size(), &result) == 0 && result != nullptr) {
        info.username = result->pw_name;
    } else {
        // If getpwuid_r fails or no entry is found, username remains "Unknown"
        // The default initialization of info.username handles this.
    }

    ::std::filesystem::path cmdlinePath = pidPath / "cmdline";
    if (auto cmdlineContentOpt = utils::readTextFile(cmdlinePath.string())) {
        ::std::string rawCmdline = *cmdlineContentOpt;
        ::std::ranges::replace(rawCmdline, '\0', ' ');
        if (!rawCmdline.empty() && rawCmdline.back() == ' ') rawCmdline.pop_back();
        info.cmdline = rawCmdline;
    } else {
        // File could not be read
        info.cmdline = "[unreadable]"; // Set specific string on error
    }
    
    try {
        info.executablePath = fs::read_symlink(pidPath / "exe").string();
    } catch (const fs::filesystem_error& e) {
        // (void)e; // Original code, suppressed error
        info.executablePath = "[unreadable]"; // Set specific string on error
    }
    try {
        info.currentWorkingDirectory = fs::read_symlink(pidPath / "cwd").string();
    } catch (const fs::filesystem_error& e) {
        // (void)e; // Original code, suppressed error
        info.currentWorkingDirectory = "[unreadable]"; // Set specific string on error
    }

    auto envResult = readProcessEnvironmentVars(procPath, pid);
    if (envResult) {
        info.environmentVariables = *envResult;
    }
    return info;
}

utils::Result<::std::vector<ProcessInfo>> getBasicSnapshot(const ProcessAnalyzer& analyzer) {
    ::std::vector<ProcessInfo> processes;
    auto pidsResult = analyzer.getPids();
    if (!pidsResult) {
        return ::std::unexpected(pidsResult.error());
    }

    for (int pid : *pidsResult) {
        auto details = analyzer.getProcessDetails(pid);
        if (details) {
            processes.push_back(*details);
        } else if (details.error() != utils::make_error_code(utils::UtilsError::analyzerProcessNotFound)) {
            return ::std::unexpected(details.error());
        }
    }
    return processes;
}



utils::Result<::std::vector<ProcessInfo>> ProcessAnalyzer::snapshot() const {
    ::std::map<int, ProcessInfo> processMap;

    auto basicProcessesResult = getBasicSnapshot(*this);
    if (!basicProcessesResult) {
        return ::std::unexpected(basicProcessesResult.error());
    }
    for (const auto& p : *basicProcessesResult) {
        processMap[p.pid] = p;
    }

    // Removed CPU usage calculation as it's a time-based metric and makes snapshot() expensive.
    // CPU usage should be fetched separately via a dedicated method.

    auto systemMemoryInfo = getSystemMemoryInfo();
    if (systemMemoryInfo.has_value() && systemMemoryInfo->memTotal > 0) {
        auto totalMemKB = static_cast<float>(systemMemoryInfo->memTotal);
        for (auto& pair : processMap) {
            ProcessInfo& info = pair.second;
            info.memoryPercentage = (static_cast<float>(info.residentMemory) / totalMemKB) * 100.0F;
        }
    }

    ::std::vector<ProcessInfo> results;
    results.reserve(processMap.size());
    for (const auto& pair : processMap) {
        results.push_back(pair.second);
    }

    return results;
}



utils::Result<SystemMemoryInfo> ProcessAnalyzer::getSystemMemoryInfo() const {
    ::std::filesystem::path meminfoPath = procPath / "meminfo";
    auto contentOpt = utils::readTextFile(meminfoPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    SystemMemoryInfo memInfo;
    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    while (::std::getline(ss, line)) {
        ::std::string key;
        unsigned long value = 0;
        ::std::stringstream lineSs(line);
        lineSs >> key >> value;
        if(key.empty()) continue;
        if (key.back() == ':') {
            key.pop_back();
        }

        if (key == "MemTotal") memInfo.memTotal = value;
        else if (key == "MemFree") memInfo.memFree = value;
        else if (key == "MemAvailable") memInfo.memAvailable = value;
        else if (key == "Buffers") memInfo.buffers = value;
        else if (key == "Cached") memInfo.cached = value;
        else if (key == "SwapTotal") memInfo.swapTotal = value;
        else if (key == "SwapFree") memInfo.swapFree = value;
    }
    return memInfo;
}

utils::Result<SystemLoadAverage> ProcessAnalyzer::getSystemLoadAverage() const {
    ::std::filesystem::path loadavgPath = procPath / "loadavg";
    auto contentOpt = utils::readTextFile(loadavgPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    SystemLoadAverage loadAvg;
    ::std::stringstream ss(*contentOpt);
    ss >> loadAvg.oneMin >> loadAvg.fiveMin >> loadAvg.fifteenMin;
    return loadAvg;
}

utils::Result<SystemCpuStats> ProcessAnalyzer::getSystemCpuStats() const {
    ::std::filesystem::path statPath = procPath / "stat";
    auto contentOpt = utils::readTextFile(statPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    ::std::getline(ss, line);

    if (line.starts_with("cpu ")) {
        SystemCpuStats stats;
        ::std::stringstream lineSs(line);
        ::std::string cpuLabel;
        lineSs >> cpuLabel >> stats.user >> stats.nice >> stats.system >> stats.idle 
                >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal;
        return stats;
    }

    return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}




utils::Result<::std::vector<ProcessInfo>> ProcessAnalyzer::queryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return ::std::unexpected(allProcessesResult.error());
    }
    auto allProcesses = *allProcessesResult;

    ::std::vector<ProcessInfo> filteredProcesses;

    ::std::ranges::copy_if(allProcesses, ::std::back_inserter(filteredProcesses),
        [&](const ProcessInfo& process) {
            return matchesFilter(process, filter);
        });

    ::std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
            return processSortPredicate(a, b, sortBy, sortOrder);
        });

    return filteredProcesses;
}

utils::Result<::std::vector<ProcessInfo>> ProcessAnalyzer::getChildProcesses(int pid) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return ::std::unexpected(allProcessesResult.error());
    }
    ::std::vector<ProcessInfo> children;
    for (const auto& process : *allProcessesResult) {
        if (process.ppid == pid) {
            children.push_back(process);
        }
    }
    return children;
}

utils::Result<ProcessCpuUsage> ProcessAnalyzer::getProcessCpuUsage(pid_t pid, ::std::chrono::milliseconds durationMs) const {
    auto startTime = ::std::chrono::high_resolution_clock::now();

    auto initialDetailsResult = getProcessDetails(pid);
    if (!initialDetailsResult) {
        return ::std::unexpected(initialDetailsResult.error());
    }
    auto initialDetails = *initialDetailsResult;

    auto initialTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!initialTotalSystemTicksResult) {
        return ::std::unexpected(initialTotalSystemTicksResult.error());
    }
    long long initialTotalSystemTicks = *initialTotalSystemTicksResult;

    ::std::this_thread::sleep_for(durationMs);

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto actualDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime); // Removed [[maybe_unused]]

    auto finalDetailsResult = getProcessDetails(pid);
    if (!finalDetailsResult) {
        // Propagate the error instead of returning a default value
        return ::std::unexpected(finalDetailsResult.error());
    }
    const auto& finalDetails = *finalDetailsResult;

    auto finalTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!finalTotalSystemTicksResult) {
        return ::std::unexpected(finalTotalSystemTicksResult.error());
    }
    long long finalTotalSystemTicks = *finalTotalSystemTicksResult;

    long long processCpuTicksDelta = (finalDetails.cpuUserTimeTicks + finalDetails.cpuKernelTimeTicks) - (initialDetails.cpuUserTimeTicks + initialDetails.cpuKernelTimeTicks);
    long long totalSystemTicksDelta = finalTotalSystemTicks - initialTotalSystemTicks;

    ProcessCpuUsage usage;
    usage.pid = pid;
    usage.name = finalDetails.name;
    if (totalSystemTicksDelta > 0) {
        usage.cpuPercentage = 100.0 * static_cast<double>(processCpuTicksDelta) / static_cast<double>(totalSystemTicksDelta);
    } else {
        usage.cpuPercentage = 0.0;
    }

    return usage;
}



utils::Result<::std::vector<::std::string>> ProcessAnalyzer::getProcessEnvironment(pid_t pid) const {
    return readProcessEnvironmentVars(procPath, pid);
}

utils::Result<ProcessInfo> ProcessAnalyzer::getParentProcess(pid_t pid) const {
    auto detailsResult = getProcessDetails(pid);
    if (!detailsResult) {
        return ::std::unexpected(detailsResult.error());
    }
    if (detailsResult->ppid == 0) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
    }
    return getProcessDetails(detailsResult->ppid);
}

utils::Result<::std::vector<ProcessInfo>> ProcessAnalyzer::getAllDescendantProcesses(pid_t pid) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return ::std::unexpected(allProcessesResult.error());
    }
    auto allProcesses = *allProcessesResult;

    ::std::vector<ProcessInfo> descendants;
    ::std::vector<int> toVisit {pid};
    ::std::map<int, ::std::vector<ProcessInfo>> parentToChildren;
    for (const auto& p : allProcesses) {
        parentToChildren[p.ppid].push_back(p);
    }
    
    size_t head = 0;
    while(head < toVisit.size()) {
        int currentPid = toVisit[head++];
        if (parentToChildren.contains(currentPid)) {
            for (const auto& child : parentToChildren.at(currentPid)) {
                descendants.push_back(child);
                toVisit.push_back(child.pid);
            }
        }
    }

    return descendants;
}

utils::Result<long> ProcessAnalyzer::getSystemClockTicksPerSecond() {
    long ticks = sysconf(_SC_CLK_TCK);
    if (ticks < 0) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
    return ticks;
}


utils::Result<void> ProcessAnalyzer::sendSignal(int pid, ProcessSignal signal) {
    if (::kill(pid, static_cast<int>(signal)) == 0) {
        return {};
    }

    utils::UtilsError errCode;
    switch (errno) {
        case ESRCH:
            errCode = utils::UtilsError::analyzerProcessNotFound;
            break;
        case EPERM:
            errCode = utils::UtilsError::analyzerPermissionDenied;
            break;
        default:
            errCode = utils::UtilsError::analyzerSystemError;
            break;
    }
    return ::std::unexpected(utils::make_error_code(errCode));
}

utils::Result<::std::vector<ThreadInfo>> ProcessAnalyzer::getProcessThreads(pid_t pid) const {
    ::std::vector<ThreadInfo> threads;
    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return ::std::unexpected(check.error());
    }
    ::std::filesystem::path taskPath = procPath / ::std::to_string(pid) / "task";

    if (!fs::exists(taskPath) || !fs::is_directory(taskPath)) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }

    try {
        for (const auto& entry : fs::directory_iterator(taskPath)) {
            if (entry.is_directory()) {
                ::std::string tidStr = entry.path().filename().string();
                if (utils::isInteger(tidStr)) {
                    int tid = ::std::stoi(tidStr);
                    ThreadInfo thread;
                    thread.tid = tid;

                    ::std::filesystem::path commPath = entry.path() / "comm";
                    if (auto commContent = utils::readTextFile(commPath.string())) {
                        thread.name = utils::trim(*commContent);
                    }

                    ::std::filesystem::path statPath = entry.path() / "stat";
                    if (auto statContentOpt = utils::readTextFile(statPath.string())) {
                        std::string content = *statContentOpt;
                        auto lastParen = content.rfind(')');
                        if (lastParen != std::string::npos) {
                            std::string fields_part = utils::trim(content.substr(lastParen + 1));
                            std::vector<std::string> stat_fields = utils::split(fields_part, ' ', true);

                            if (!stat_fields.empty()) {
                                thread.state = stat_fields[0][0];
                            }

                            unsigned long utime = 0, stime = 0;
                            if (stat_fields.size() > 12) {
                                utils::tryParse(stat_fields[11], utime);
                                utils::tryParse(stat_fields[12], stime);
                                thread.cpuUserTimeTicks = utime;
                                thread.cpuKernelTimeTicks = stime;
                            }
                        }
                    }
                    threads.push_back(thread);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return threads;
}

utils::Result<::std::vector<MountPointInfo>> ProcessAnalyzer::getSystemDiskUsage() const {
    ::std::vector<MountPointInfo> mounts;
    ::std::filesystem::path mountsPath = procPath / "mounts";
    auto contentOpt = utils::readTextFile(mountsPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    while(::std::getline(ss, line)) {
        ::std::stringstream lineSs(line);
        ::std::string device;
        ::std::string mountPoint;
        ::std::string filesystemType;
        lineSs >> device >> mountPoint >> filesystemType;

        struct statvfs vfs;
        if (statvfs(mountPoint.c_str(), &vfs) == 0) {
            mounts.push_back({
                .mountPoint = mountPoint,
                .filesystemType = filesystemType,
                .device = device,
                .totalSpaceBytes = (unsigned long long)vfs.f_blocks * vfs.f_frsize,
                .freeSpaceBytes = (unsigned long long)vfs.f_bfree * vfs.f_frsize,
                .availableSpaceBytes = (unsigned long long)vfs.f_bavail * vfs.f_frsize
            });
        }
    }
    return mounts;
}





utils::Result<ProcessDiskIoUsage> ProcessAnalyzer::getProcessDiskIoUsage(pid_t pid, ::std::chrono::milliseconds durationMs) const {
    auto startTime = ::std::chrono::high_resolution_clock::now();

    auto initialDetailsResult = getProcessDetails(pid);
    if (!initialDetailsResult) return ::std::unexpected(initialDetailsResult.error());
    auto initialDetails = *initialDetailsResult;

    ::std::this_thread::sleep_for(durationMs);

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto actualDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);
    
    auto finalDetailsResult = getProcessDetails(pid);
    if (!finalDetailsResult) {
        return ProcessDiskIoUsage {
            .pid = pid,
            .name = initialDetails.name,
            .readBytesPerSecond = 0.0,
            .writeBytesPerSecond = 0.0
        };
    }
    auto finalDetails = *finalDetailsResult;

    auto readDelta = static_cast<double>(finalDetails.ioReadBytes - initialDetails.ioReadBytes);
    auto writeDelta = static_cast<double>(finalDetails.ioWriteBytes - initialDetails.ioWriteBytes);
    double durationSec = static_cast<double>(actualDuration.count()) / msInSecond;

    return ProcessDiskIoUsage {
        .pid = pid,
        .name = finalDetails.name,
        .readBytesPerSecond = durationSec > 0 ? readDelta / durationSec : 0.0,
        .writeBytesPerSecond = durationSec > 0 ? writeDelta / durationSec : 0.0
    };
}

utils::Result<::std::vector<ProcessDiskIoUsage>> ProcessAnalyzer::getAllProcessesDiskIoUsage(::std::chrono::milliseconds durationMs) const {
    auto startTime = ::std::chrono::high_resolution_clock::now();

    auto initialSnapshotResult = snapshot();
    if (!initialSnapshotResult) return ::std::unexpected(initialSnapshotResult.error());
    auto initialSnapshot = *initialSnapshotResult;

    ::std::this_thread::sleep_for(durationMs);

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto actualDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime);

    auto finalSnapshotResult = snapshot();
    if (!finalSnapshotResult) return ::std::unexpected(finalSnapshotResult.error());
    auto finalSnapshot = *finalSnapshotResult;

    ::std::map<int, ProcessInfo> finalSnapshotMap;
    for(const auto& info : finalSnapshot) {
        finalSnapshotMap[info.pid] = info;
    }

    ::std::vector<ProcessDiskIoUsage> results;
    double durationSec = static_cast<double>(actualDuration.count()) / msInSecond;
    if (durationSec <= 0) return results;

    for(const auto& initialInfo : initialSnapshot) {
        auto it = finalSnapshotMap.find(initialInfo.pid);
        if (it != finalSnapshotMap.end()) {
            const auto& finalInfo = it->second;
            auto readDelta = static_cast<double>(finalInfo.ioReadBytes - initialInfo.ioReadBytes);
            auto writeDelta = static_cast<double>(finalInfo.ioWriteBytes - initialInfo.ioWriteBytes);
            results.push_back({
                .pid = finalInfo.pid,
                .name = finalInfo.name,
                .readBytesPerSecond = readDelta / durationSec,
                .writeBytesPerSecond = writeDelta / durationSec
            });
        }
    }
    return results;
}

utils::Result<::std::vector<OpenFileDescriptorInfo>> ProcessAnalyzer::getProcessOpenFileDetails(pid_t pid) const {
    ::std::vector<OpenFileDescriptorInfo> openFds;
    ::std::filesystem::path fdPath = procPath / ::std::to_string(pid) / "fd";

    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return ::std::unexpected(check.error());
    }

    // Use std::unique_ptr with a custom deleter for DIR* to ensure closedir is called (RAII)
    auto dir_closer = [](DIR* d){ if(d) closedir(d); };
    std::unique_ptr<DIR, decltype(dir_closer)> dir_ptr(opendir(fdPath.c_str()), dir_closer);
    
    if (!dir_ptr) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    int dirFd = dirfd(dir_ptr.get());

    try {
        for (const auto& entry : fs::directory_iterator(fdPath)) {
            if (!entry.is_symlink()) continue;

            ::std::string fdStr = entry.path().filename().string();
            if (!utils::isInteger(fdStr)) continue;

            int fd = ::std::stoi(fdStr);
            OpenFileDescriptorInfo info;
            info.fd = fd;

            try {
                info.path = fs::read_symlink(entry.path()).string();
            } catch (const fs::filesystem_error&) {
                info.path = "[unreadable]";
                info.type = OpenFileType::Unknown;
                openFds.push_back(info);
                continue;
            }

            if (info.path.starts_with("socket:")) {
                info.type = OpenFileType::Socket;
            } else if (info.path.starts_with("pipe:")) {
                info.type = OpenFileType::Pipe;
            } else if (info.path.starts_with("anon_inode:")) {
                info.type = OpenFileType::AnonInode;
            } else {
                struct stat statbuf;
                if (fstatat(dirFd, fdStr.c_str(), &statbuf, AT_SYMLINK_NOFOLLOW) == 0) {
                    if (S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) info.type = OpenFileType::File;
                    else if (S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode)) info.type = OpenFileType::Device;
                    else if (S_ISFIFO(statbuf.st_mode)) info.type = OpenFileType::Pipe;
                    else if (S_ISSOCK(statbuf.st_mode)) info.type = OpenFileType::Socket;
                    else info.type = OpenFileType::Other;
                } else {
                    info.type = OpenFileType::Unknown;
                }
            }
            openFds.push_back(info);
        }
    } catch (const fs::filesystem_error& e) {
        // The unique_ptr will automatically call closedir here, so no manual call needed.
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    
    // The unique_ptr will automatically call closedir here.
    return openFds;
}

utils::Result<::std::vector<NetworkConnection>> ProcessAnalyzer::getNetworkConnections(int pid) const {
    auto fdsResult = getProcessOpenFileDetails(pid);
    if (!fdsResult) {
        return ::std::unexpected(fdsResult.error());
    }

    ::std::set<int> socketInodes;
    constexpr int socketInodePrefixLen = 8; // "socket:["
    constexpr int socketInodeSuffixLen = 1; // "]"

    for (const auto& fdInfo : *fdsResult) {
        if (fdInfo.type == OpenFileType::Socket) {
            if (fdInfo.path.starts_with("socket:[")) {
                ::std::string inodeStr = fdInfo.path.substr(socketInodePrefixLen, fdInfo.path.length() - socketInodePrefixLen - socketInodeSuffixLen);
                if(utils::isInteger(inodeStr)){
                    socketInodes.insert(::std::stoi(inodeStr));
                }
            }
        }
    }

    ::std::vector<NetworkConnection> connections;
    ::std::filesystem::path netPath = procPath / "net";

    // The parseNetFileHelper is still largely unimplemented, so these calls will return empty vectors for now.
    // This part of the fix primarily ensures the correct function signatures and path handling.
    // The actual parsing logic and filtering by socketInodes will be implemented in a future iteration.
    
    // TCP IPv4
    auto tcp4_internal = parseNetFileHelper(netPath / "tcp", "TCP");
    for(const auto& conn : tcp4_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    // TCP IPv6
    auto tcp6_internal = parseNetFileHelper(netPath / "tcp6", "TCP6");
    for(const auto& conn : tcp6_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    // UDP IPv4
    auto udp4_internal = parseNetFileHelper(netPath / "udp", "UDP");
    for(const auto& conn : udp4_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    // UDP IPv6
    auto udp6_internal = parseNetFileHelper(netPath / "udp6", "UDP6");
    for(const auto& conn : udp6_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    return connections;
}


// --- C++23 Streaming API Implementations ---

std::generator<int> ProcessAnalyzer::streamPids() const {
    if (!fs::exists(procPath)) {
        co_return; 
    }

    for (const auto& entry : fs::directory_iterator(procPath)) {
        if (entry.is_directory()) {
            ::std::string filename = entry.path().filename().string();
            if (utils::isInteger(filename)) {
                co_yield ::std::stoi(filename);
            }
        }
    }
}

std::generator<ProcessInfo> ProcessAnalyzer::streamProcesses() const {
    for (int pid : streamPids()) {
        auto details = getProcessDetails(pid);
        if (details.has_value()) {
            co_yield *details;
        }
    }
}

std::generator<ProcessInfo> ProcessAnalyzer::streamQueryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    // Note: This generator materializes all filtered processes into a vector before sorting and yielding.
    // This is a necessary trade-off for providing sorted results. For large datasets, this can consume
    // significant memory. If lazy filtering without sorting is required, consider using streamProcesses()
    // with a custom filter.
    ::std::vector<ProcessInfo> filteredProcesses;
    for (const auto& process : streamProcesses()) {
         if (matchesFilter(process, filter)) {
             filteredProcesses.push_back(process);
         }
    }
    
    ::std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
            return processSortPredicate(a, b, sortBy, sortOrder);
        });

    for(auto& process : filteredProcesses){
        co_yield process;
    }
}

utils::Result<::std::vector<MemoryMapInfo>> ProcessAnalyzer::getProcessMemoryMaps(pid_t pid) const {
    ::std::vector<MemoryMapInfo> maps;
    ::std::filesystem::path mapsPath = procPath / ::std::to_string(pid) / "maps";

    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return ::std::unexpected(check.error());
    }

    auto contentOpt = utils::readTextFile(mapsPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    while (::std::getline(ss, line)) {
        MemoryMapInfo mapInfo;
        char dash;
        
        ::std::stringstream lineSs(line);
        lineSs >> ::std::hex >> mapInfo.startAddress >> dash >> mapInfo.endAddress >> ::std::dec;
        lineSs >> mapInfo.permissions;
        lineSs >> ::std::hex >> mapInfo.offset >> ::std::dec;
        lineSs >> mapInfo.device;
        lineSs >> mapInfo.inode;
        
        ::std::string pathname;
        if (::std::getline(lineSs, pathname)) {
             mapInfo.pathname = utils::trim(pathname);
        } else {
             mapInfo.pathname = "";
        }

        maps.push_back(mapInfo);
    }

    return maps;
}

utils::Result<ResourceLimitInfo> ProcessAnalyzer::getProcessResourceLimits(pid_t pid) const {
    ResourceLimitInfo limitInfo;
    ::std::filesystem::path limitsPath = procPath / ::std::to_string(pid) / "limits";

    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return ::std::unexpected(check.error());
    }

    auto contentOpt = utils::readTextFile(limitsPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    ::std::getline(ss, line); 

    ::std::string header = line;
    size_t softLimitPos = header.find("Soft Limit");
    size_t hardLimitPos = header.find("Hard Limit");
    size_t unitsPos = header.find("Units");

    if (softLimitPos == ::std::string::npos || hardLimitPos == ::std::string::npos || unitsPos == ::std::string::npos) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    while (::std::getline(ss, line)) {
        if (line.length() < unitsPos) continue;
        ResourceLimit limit;
        
        limit.resource = utils::trim(line.substr(0, softLimitPos));
        limit.softLimit = utils::trim(line.substr(softLimitPos, hardLimitPos - softLimitPos));
        limit.hardLimit = utils::trim(line.substr(hardLimitPos, unitsPos - hardLimitPos));
        limit.units = utils::trim(line.substr(unitsPos));

        if (limit.resource.empty()) continue;

        limitInfo.limits.push_back(limit);
    }

    return limitInfo;
}

utils::Result<CgroupInfo> ProcessAnalyzer::getProcessCgroupInfo(pid_t pid) const {
    CgroupInfo cgroupInfo;
    ::std::filesystem::path cgroupPath = procPath / ::std::to_string(pid) / "cgroup";

    auto check = checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return ::std::unexpected(check.error());
    }

    auto contentOpt = utils::readTextFile(cgroupPath.string());
     if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }
    
    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    while(::std::getline(ss, line)) {
        CgroupEntry entry;
        char colon;
        ::std::stringstream lineSs(line);
        lineSs >> entry.id >> colon;
        ::std::getline(lineSs, entry.controllers, ':');
        ::std::getline(lineSs, entry.path);
        cgroupInfo.entries.push_back(entry);
    }

    return cgroupInfo;
}

// Implementations for NEW API methods
utils::Result<void> ProcessAnalyzer::setProcessNiceness(pid_t pid, int niceness) {
    if (setpriority(PRIO_PROCESS, pid, niceness) == 0) {
        return {};
    }
    return ::std::unexpected(utils::make_error_code((errno == EACCES || errno == EPERM) ? utils::UtilsError::analyzerPermissionDenied : utils::UtilsError::analyzerSystemError));
}

utils::Result<void> ProcessAnalyzer::setProcessCpuAffinity(pid_t pid, const CpuSet& affinity) {
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int cpu : affinity.cpus) {
        CPU_SET(cpu, &set);
    }
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &set) == 0) {
        return {};
    }
    return ::std::unexpected(utils::make_error_code((errno == EACCES || errno == EPERM) ? utils::UtilsError::analyzerPermissionDenied : utils::UtilsError::analyzerSystemError));
}

utils::Result<PerCpuUsage> ProcessAnalyzer::getPerCpuUsage(::std::chrono::milliseconds durationMs) const {
    struct CpuSnapshot {
        unsigned long long total;
        unsigned long long idle;
    };
    
    auto getSnapshots = [&](const ::std::filesystem::path& path) -> ::std::vector<CpuSnapshot> {
        ::std::vector<CpuSnapshot> snapshots;
        auto contentOpt = utils::readTextFile(path.string());
        if (!contentOpt) return snapshots;

        ::std::stringstream ss(*contentOpt);
        ::std::string line;
        while(::std::getline(ss, line)) {
             if (line.starts_with("cpu")) {
                ::std::stringstream lineSs(line);
                ::std::string label;
                lineSs >> label;
                
                unsigned long long user;
                unsigned long long nice;
                unsigned long long system;
                unsigned long long idle;
                unsigned long long iowait;
                unsigned long long irq;
                unsigned long long softirq;
                unsigned long long steal;
                lineSs >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
                 
                snapshots.push_back({
                    .total = user + nice + system + idle + iowait + irq + softirq + steal,
                    .idle = idle
                });
             }
        }
        return snapshots;
    };

    auto startTime = ::std::chrono::high_resolution_clock::now();

    ::std::filesystem::path statPath = procPath / "stat";
    auto initialSnapshots = getSnapshots(statPath);
    if (initialSnapshots.empty()) return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));

    ::std::this_thread::sleep_for(durationMs);

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto actualDuration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime); // Removed [[maybe_unused]]

    auto finalSnapshots = getSnapshots(statPath);
    if (finalSnapshots.empty()) return ::std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));

    PerCpuUsage usage;
    for (size_t i = 0; i < initialSnapshots.size() && i < finalSnapshots.size(); ++i) {
        auto totalDelta = finalSnapshots[i].total - initialSnapshots[i].total;
        auto idleDelta = finalSnapshots[i].idle - initialSnapshots[i].idle;
        
        double pct = 0.0;
        if (totalDelta > 0) {
            pct = 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta));
        }
        usage.cpuUsages.push_back({static_cast<int>(i), pct});
    }
    return usage;
}

utils::Result<::std::vector<DiskIoDeviceStats>> ProcessAnalyzer::getSystemDiskIoStats() const {
    ::std::vector<DiskIoDeviceStats> stats;
    ::std::filesystem::path diskStatsPath = procPath / "diskstats";
    auto contentOpt = utils::readTextFile(diskStatsPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }
    
    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    while(::std::getline(ss, line)) {
        ::std::stringstream lineSs(line);
        int major;
        int minor;
        ::std::string deviceName;
        lineSs >> major >> minor >> deviceName;
        
        DiskIoDeviceStats stat;
        stat.deviceName = deviceName;
        lineSs >> stat.readsCompleted >> stat.readsMerged >> stat.sectorsRead >> stat.readTimeMs
               >> stat.writesCompleted >> stat.writesMerged >> stat.sectorsWritten >> stat.writeTimeMs
               >> stat.ioProgressMs >> stat.ioWeightedTimeMs;
        stats.push_back(stat);
    }
    return stats;
}

utils::Result<::std::vector<NetworkInterfaceStats>> ProcessAnalyzer::getNetworkInterfaceStats() const {
    ::std::vector<NetworkInterfaceStats> stats;
    ::std::filesystem::path netDevPath = procPath / "net" / "dev";
    auto contentOpt = utils::readTextFile(netDevPath.string());
    if (!contentOpt) {
        return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    ::std::getline(ss, line); // header 1
    ::std::getline(ss, line); // header 2

    while(::std::getline(ss, line)) {
        ::std::stringstream lineSs(line);
        ::std::string interfaceName;
        ::std::getline(lineSs, interfaceName, ':');
        interfaceName = utils::trim(interfaceName);
        
        NetworkInterfaceStats stat;
        stat.interfaceName = interfaceName;
        
        unsigned long dummy1, dummy2, dummy3, dummy4; // For skipping fifo, frame, compressed, multicast
        lineSs >> stat.rxBytes >> stat.rxPackets >> stat.rxErrors >> stat.rxDropped
               >> dummy1 >> dummy2 >> dummy3 >> dummy4 // skipping fifo, frame, compressed, multicast
               >> stat.txBytes >> stat.txPackets >> stat.txErrors >> stat.txDropped;
               
        stats.push_back(stat);
    }
    return stats;
}

utils::Result<SystemActivityStats> ProcessAnalyzer::getSystemActivityStats() const {
    SystemActivityStats stats;
    stats.interruptsTotal = 0;
    stats.contextSwitches = 0;
    stats.processesForked = 0;

    ::std::filesystem::path statPath = procPath / "stat";
    auto contentOpt = utils::readTextFile(statPath.string());
    if (!contentOpt) {
         return ::std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    ::std::stringstream ss(*contentOpt);
    ::std::string line;
    while(::std::getline(ss, line)) {
        if (line.starts_with("intr ")) {
            ::std::stringstream lineSs(line);
            ::std::string label;
            lineSs >> label >> stats.interruptsTotal;
        } else if (line.starts_with("ctxt ")) {
             ::std::stringstream lineSs(line);
             ::std::string label;
             lineSs >> label >> stats.contextSwitches;
        } else if (line.starts_with("processes ")) {
             ::std::stringstream lineSs(line);
             ::std::string label;
             lineSs >> label >> stats.processesForked;
        }
    }
    return stats;
}
