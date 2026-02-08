// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "Analyzer.h"
#include "utils.h"
#include <iostream>
#include <filesystem>

#include <sstream>
#include <algorithm>
#include <ranges> // For std::ranges::copy_if
#include <pwd.h>        // For getpwuid
#include <sys/types.h>
#include <sys/stat.h>   // For fstatat, S_ISREG, etc.
#include <string_view> // For std::string_view::starts_with
#include <chrono>
#include <thread>
#include <stdexcept>
#include <cmath>

#include <unistd.h>     // For sysconf, gethostname
#include <csignal>     // For kill
#include <sys/statvfs.h> // for statvfs
#include <netinet/in.h> // for INET6_ADDRSTRLEN
#include <arpa/inet.h>  // for inet_ntop
#include <fcntl.h>      // For fstatat
#include <dirent.h>     // For dirfd, opendir
#include <regex>

#include <set>

namespace fs = std::filesystem;

namespace {
    // Number of fields to skip after ppid (4th field) and before utime (14th field)
    // Fields skipped: pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt
    // Count: 9 fields (from 5th to 13th, 1-based indexing)
    constexpr int statFieldsToSkipBeforeUtime = 9;

    // Number of fields to skip after nice (19th field) and before starttime (22nd field)
    // Fields skipped: num_threads, itrealvalue
    // Count: 3 fields (from 19th to 21st, 1-based indexing)
    constexpr int statFieldsToSkipBeforeStarttime = 3;
    constexpr int statFieldsToSkipBeforeThreadUtime = 10;
    constexpr double kMSInSecond = 1000.0;
    constexpr long kDefaultSystemClockTicks = 100;


    std::expected<long long, AnalyzerErrorDetail> getTotalSystemCpuTimeTicks(std::string_view procPath) {
        auto stats = ProcessAnalyzer(procPath).getSystemCpuStats();
        if(stats) {
            return static_cast<long long>(stats->user) + static_cast<long long>(stats->nice) +
                   static_cast<long long>(stats->system) + static_cast<long long>(stats->idle) +
                   static_cast<long long>(stats->iowait) + static_cast<long long>(stats->irq) +
                   static_cast<long long>(stats->softirq) + static_cast<long long>(stats->steal);
        }
        return std::unexpected(stats.error());
    }

    constexpr int kExamplePid1 = 100;
    constexpr int kExamplePid2 = 200;

    // Helper function to get system boot time in Unix timestamp (seconds since epoch)
    long long getSystemBootTimeUnix(std::string_view procPath) {
        std::string uptimePath = std::string(procPath) + "/uptime";
        auto contentOpt = utils::readTextFile(uptimePath);
        if (!contentOpt) {
            // Log or handle error appropriately. For now, return 0. 
            return 0; 
        }
        std::stringstream ss(*contentOpt);
        double uptimeSeconds;
        ss >> uptimeSeconds;

        auto now = std::chrono::system_clock::now();
        long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        return currentTimeUnix - static_cast<long long>(uptimeSeconds);
    }
    
    // Internal struct to temporarily hold NetworkConnection data along with its inode
    // for filtering purposes before converting to the public NetworkConnection struct.
    struct InternalNetworkConnection {
        NetworkConnection baseConn;
        int inode;
    };

    enum class TcpState : std::uint8_t {
        kEstablished = 1,
        kSynSent,
        kSynRecv,
        kFinWait1,
        kFinWait2,
        kTimeWait,
        kClose,
        kCloseWait,
        kLastAck,
        kListen,
        kClosing,
        kUnknown
    };
    const int kIpv6LineDummyCount = 5;

    // Helper function to parse /proc/net/tcp, udp, etc. files.
    std::vector<InternalNetworkConnection> parseNetFileHelper(const std::string& filePath, std::string_view protocolPrefix) {
        std::vector<InternalNetworkConnection> internalConnections;
        auto content = utils::readTextFile(filePath);
        if (!content) return internalConnections;

        std::stringstream ss(*content);
        std::string line;
        std::getline(ss, line); // Skip header

        while (std::getline(ss, line)) {
            std::stringstream lineSs(line);
            InternalNetworkConnection internalConn;
            internalConn.baseConn.protocol = protocolPrefix;

            int dummy;
            unsigned long localAddrPart1 = 0;
            unsigned long localAddrPart2 = 0;
            unsigned long localAddrPart3 = 0;
            unsigned long localAddrPart4 = 0; // For IPv6
            unsigned long remoteAddrPart1 = 0;
            unsigned long remoteAddrPart2 = 0;
            unsigned long remoteAddrPart3 = 0;
            unsigned long remoteAddrPart4 = 0; // For IPv6
            unsigned long localAddr = 0; // For IPv4
            unsigned long remoteAddr = 0; // For IPv4
            unsigned int localP = 0;
            unsigned int remoteP = 0;
            int state = 0;
            char colon;

            lineSs >> dummy; 
            lineSs.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // Skip 'sl' column

            if (filePath.find('6') != std::string::npos) { // IPv6
                lineSs >> std::hex >> localAddrPart1 >> localAddrPart2 >> localAddrPart3 >> localAddrPart4 >> colon >> localP
                       >> remoteAddrPart1 >> remoteAddrPart2 >> remoteAddrPart3 >> remoteAddrPart4 >> colon >> remoteP
                       >> state;
                for(int i=0; i<kIpv6LineDummyCount; ++i) lineSs >> dummy;
                lineSs >> internalConn.inode;
                
                std::array<char, INET6_ADDRSTRLEN> localStrBuf{};
                std::array<char, INET6_ADDRSTRLEN> remoteStrBuf{};
                
                in6_addr localIn6Addr;
                localIn6Addr.__in6_u.__u6_addr32[0] = localAddrPart1;
                localIn6Addr.__in6_u.__u6_addr32[1] = localAddrPart2;
                localIn6Addr.__in6_u.__u6_addr32[2] = localAddrPart3;
                localIn6Addr.__in6_u.__u6_addr32[3] = localAddrPart4;

                inet_ntop(AF_INET6, &localIn6Addr, localStrBuf.data(), localStrBuf.size());
                internalConn.baseConn.localAddress = std::string(localStrBuf.data()) + ":" + std::to_string(localP);
                
                if(remoteP > 0 || (remoteAddrPart1 || remoteAddrPart2 || remoteAddrPart3 || remoteAddrPart4)) {
                    in6_addr remoteIn6Addr;
                    remoteIn6Addr.__in6_u.__u6_addr32[0] = remoteAddrPart1;
                    remoteIn6Addr.__in6_u.__u6_addr32[1] = remoteAddrPart2;
                    remoteIn6Addr.__in6_u.__u6_addr32[2] = remoteAddrPart3;
                    remoteIn6Addr.__in6_u.__u6_addr32[3] = remoteAddrPart4;
                    inet_ntop(AF_INET6, &remoteIn6Addr, remoteStrBuf.data(), remoteStrBuf.size());
                    internalConn.baseConn.remoteAddress = std::string(remoteStrBuf.data()) + ":" + std::to_string(remoteP);
                } else {
                    internalConn.baseConn.remoteAddress = "*";
                }

            } else { // IPv4
                lineSs >> std::hex >> localAddr >> colon >> localP
                       >> remoteAddr >> colon >> remoteP
                       >> state >> dummy >> dummy >> dummy >> dummy >> dummy >> internalConn.inode;
                
                std::array<char, INET_ADDRSTRLEN> localStrBuf{};
                std::array<char, INET_ADDRSTRLEN> remoteStrBuf{};
                
                struct in_addr localInAddr = { .s_addr = static_cast<in_addr_t>(localAddr) };
                struct in_addr remoteInAddr = { .s_addr = static_cast<in_addr_t>(remoteAddr) };
                inet_ntop(AF_INET, &localInAddr, localStrBuf.data(), localStrBuf.size());
                inet_ntop(AF_INET, &remoteInAddr, remoteStrBuf.data(), remoteStrBuf.size());
                
                internalConn.baseConn.localAddress = std::string(localStrBuf.data()) + ":" + std::to_string(localP);
                
                if(remoteP > 0 || remoteAddr != 0) {
                    internalConn.baseConn.remoteAddress = std::string(remoteStrBuf.data()) + ":" + std::to_string(remoteP);
                } else {
                    internalConn.baseConn.remoteAddress = "*";
                }
            }

            if (protocolPrefix.starts_with("TCP")) {
                switch(static_cast<TcpState>(state)){
                    case TcpState::kEstablished: internalConn.baseConn.state = "ESTABLISHED"; break;
                    case TcpState::kSynSent: internalConn.baseConn.state = "SYN_SENT"; break;
                    case TcpState::kSynRecv: internalConn.baseConn.state = "SYN_RECV"; break;
                    case TcpState::kFinWait1: internalConn.baseConn.state = "FIN_WAIT1"; break;
                    case TcpState::kFinWait2: internalConn.baseConn.state = "FIN_WAIT2"; break;
                    case TcpState::kTimeWait: internalConn.baseConn.state = "TIME_WAIT"; break;
                    case TcpState::kClose: internalConn.baseConn.state = "CLOSE"; break;
                    case TcpState::kCloseWait: internalConn.baseConn.state = "CLOSE_WAIT"; break;
                    case TcpState::kLastAck: internalConn.baseConn.state = "LAST_ACK"; break;
                    case TcpState::kListen: internalConn.baseConn.state = "LISTEN"; break;
                    case TcpState::kClosing: internalConn.baseConn.state = "CLOSING"; break;
                    default: internalConn.baseConn.state = "UNKNOWN"; break;
                }
            } else {
                internalConn.baseConn.state = "UNCONN";
            }
            internalConnections.push_back(internalConn);
        }
        return internalConnections;
    }

    // Helper function to apply process filters
    bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter) {
        if (filter.nameContains && process.name.find(*filter.nameContains) == std::string::npos) return false;
        if (filter.nameRegex && !std::regex_search(process.name, *filter.nameRegex)) return false;
        
        if (filter.userFilter && process.username != *filter.userFilter) return false;
        if (filter.stateFilter && !process.state.empty() && process.state[0] != *filter.stateFilter) return false;
        
        if (filter.minThreads && process.threadCount < *filter.minThreads) return false;
        if (filter.maxThreads && process.threadCount > *filter.maxThreads) return false;
        
        if (filter.minResidentMemoryKB && process.residentMemory < *filter.minResidentMemoryKB) return false;
        if (filter.maxResidentMemoryKB && process.residentMemory > *filter.maxResidentMemoryKB) return false;
        if (filter.minVirtualMemoryKB && process.virtualMemory < *filter.minVirtualMemoryKB) return false;
        if (filter.maxVirtualMemoryKB && process.virtualMemory > *filter.maxVirtualMemoryKB) return false;
        
        if (filter.cmdlineContains && process.cmdline.find(*filter.cmdlineContains) == std::string::npos) return false;
        if (filter.cmdlineRegex && !std::regex_search(process.cmdline, *filter.cmdlineRegex)) return false;
        
        if (filter.executablePathContains && process.executablePath.find(*filter.executablePathContains) == std::string::npos) return false;
        if (filter.executablePathRegex && !std::regex_search(process.executablePath, *filter.executablePathRegex)) return false;
        
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

    std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> getBasicSnapshot(const ProcessAnalyzer& analyzer) {
        std::vector<ProcessInfo> processes;
        auto pidsResult = analyzer.getPids();
        if (!pidsResult) {
            return std::unexpected(pidsResult.error());
        }

        for (int pid : *pidsResult) {
            auto details = analyzer.getProcessDetails(pid);
            if (details) {
                processes.push_back(*details);
            } else if (details.error().code != AnalyzerError::processNotFound) {
                return std::unexpected(details.error());
            }
        }
        return processes;
    }

} // Anonymous namespace ends


ProcessAnalyzer::ProcessAnalyzer(std::string_view procPath) : procPath(procPath) {}

std::expected<std::vector<int>, AnalyzerErrorDetail> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Process directory not found at " + std::string(procPath),
            .systemErrno = std::nullopt
        });
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (utils::isInteger(filename)) {
                    pids.push_back(std::stoi(filename));
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::permissionDenied,
            .message = "Failed to iterate process directory: " + std::string(e.what()),
            .systemErrno = e.code().value()
        });
    }
    return pids;
}

std::expected<ProcessInfo, AnalyzerErrorDetail> ProcessAnalyzer::getProcessDetails(int pid) const {
    ProcessInfo info;
    info.pid = pid;
    info.name = "Unknown";
    info.state = "?";
    info.residentMemory = 0;
    info.virtualMemory = 0;
    info.ppid = 0;
    info.uid = 0;
    info.username = "Unknown";
    info.threadCount = 0;
    info.cmdline = "";
    info.startTimeTicks = 0;
    info.executablePath = "";
    info.currentWorkingDirectory = "";
    info.cpuUserTimeTicks = 0;
    info.cpuKernelTimeTicks = 0;
    info.ioReadBytes = 0;
    info.ioWriteBytes = 0;
    info.priority = 0;
    info.startTimeUnix = 0;
    info.elapsedTime = "N/A";
    info.cpuUsage = 0.0F;
    info.memoryPercentage = 0.0F;

    std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
    if (!fs::exists(pidPath)) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::processNotFound,
            .message = "Process with PID " + std::to_string(pid) + " not found.",
            .systemErrno = std::nullopt
        });
    }

    std::string statusPath = pidPath + "/status";
    if (auto statusContentOpt = utils::readTextFile(statusPath)) {
        std::vector<std::string> lines = utils::split(*statusContentOpt, '\n');
        for (const auto& line : lines) {
            if (line.starts_with("Name:")) {
                info.name = line.substr(line.find(':') + 1);
                info.name.erase(0, info.name.find_first_not_of(" \t"));
            } else if (line.starts_with("State:")) {
                info.state = line.substr(line.find(':') + 1);
                info.state.erase(0, info.state.find_first_not_of(" \t"));
            } else if (line.starts_with("VmSize:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.virtualMemory;
            } else if (line.starts_with("VmRSS:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.residentMemory;
            } else if (line.starts_with("PPid:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.ppid;
            } else if (line.starts_with("Uid:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.uid;
            }
            else if (line.starts_with("Threads:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.threadCount;
            }
        }
    } else {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read status file for PID " + std::to_string(pid),
            .systemErrno = errno
        });
    }

    std::string statPath = pidPath + "/stat";
    if (auto statContentOpt = utils::readTextFile(statPath)) {
        std::stringstream ss(*statContentOpt);
        std::string comm;
        char state;
        ss >> info.pid >> comm >> state >> info.ppid; 
        long long utime;
        long long stime;
        long long cutime;
        long long cstime;
        long priority;
        long nice;
        for (int i = 0; i < statFieldsToSkipBeforeUtime; ++i) { std::string dummy; ss >> dummy; }
        ss >> utime >> stime >> cutime >> cstime;
        info.cpuUserTimeTicks = utime + cutime;
        info.cpuKernelTimeTicks = stime + cstime;
        ss >> priority >> nice;
        info.priority = static_cast<int>(nice);
        for (int i = 0; i < (statFieldsToSkipBeforeStarttime-1); ++i) { std::string dummy; ss >> dummy; }
        ss >> info.startTimeTicks;
    }

    long long systemBootTimeUnix = getSystemBootTimeUnix(procPath);
    if (systemBootTimeUnix != 0) {
        long systemClockTicks = getSystemClockTicksPerSecond().value_or(kDefaultSystemClockTicks); 
        if (systemClockTicks > 0) {
            long long processStartTimeSec = info.startTimeTicks / systemClockTicks;
            info.startTimeUnix = systemBootTimeUnix + processStartTimeSec;
            
            auto now = std::chrono::system_clock::now();
            long long currentTimeUnix = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            long long elapsedSeconds = currentTimeUnix - info.startTimeUnix;
            info.elapsedTime = utils::formatElapsedTime(elapsedSeconds);
        }
    }

    std::string ioPath = pidPath + "/io";
    if (auto ioContentOpt = utils::readTextFile(ioPath)) {
        std::vector<std::string> lines = utils::split(*ioContentOpt, '\n');
        for (const auto& line : lines) {
            if (line.starts_with("rchar:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.ioReadBytes;
            } else if (line.starts_with("wchar:")) {
                std::stringstream(line.substr(line.find(':') + 1)) >> info.ioWriteBytes;
            }
        }
    }

    if (struct passwd *pw = getpwuid(info.uid)) {
        info.username = pw->pw_name;
    }

    std::string cmdlinePath = pidPath + "/cmdline";
    if (auto cmdlineContentOpt = utils::readTextFile(cmdlinePath)) {
        std::string rawCmdline = *cmdlineContentOpt;
        std::ranges::replace(rawCmdline, '\0', ' ');
        if (!rawCmdline.empty() && rawCmdline.back() == ' ') rawCmdline.pop_back();
        info.cmdline = rawCmdline;
    }
    
    try {
        info.executablePath = fs::read_symlink(pidPath + "/exe").string();
    } catch (const fs::filesystem_error& e) { (void)e; }
    try {
        info.currentWorkingDirectory = fs::read_symlink(pidPath + "/cwd").string();
    } catch (const fs::filesystem_error& e) { (void)e; }

    std::string environPath = pidPath + "/environ";
    if (auto environContentOpt = utils::readTextFile(environPath)) {
        std::string_view content = *environContentOpt;
        size_t start = 0;
        while(start < content.size()) {
            size_t end = content.find('\0', start);
            if (end == std::string_view::npos) break;
            info.environmentVariables.emplace_back(content.substr(start, end - start));
            start = end + 1;
        }
    }

    return info;
}

std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> ProcessAnalyzer::snapshot() const {
    std::map<int, ProcessInfo> processMap;

    auto basicProcessesResult = getBasicSnapshot(*this);
    if (!basicProcessesResult) {
        return std::unexpected(basicProcessesResult.error());
    }
    for (const auto& p : *basicProcessesResult) {
        processMap[p.pid] = p;
    }

    constexpr int kCpuDurationMs = 100;
    auto cpuDuration = std::chrono::milliseconds(kCpuDurationMs);

    auto allCpuUsageResult = getAllProcessesCpuUsage(cpuDuration);
    if (allCpuUsageResult) {
        for (const auto& cpuUsage : *allCpuUsageResult) {
            if (processMap.contains(cpuUsage.pid)) {
                processMap[cpuUsage.pid].cpuUsage = static_cast<float>(cpuUsage.cpuPercentage);
            }
        }
    }

    auto systemMemoryInfo = getSystemMemoryInfo();
    if (systemMemoryInfo.has_value() && systemMemoryInfo->memTotal > 0) {
        auto totalMemKB = static_cast<float>(systemMemoryInfo->memTotal);
        for (auto& pair : processMap) {
            ProcessInfo& info = pair.second;
            info.memoryPercentage = (static_cast<float>(info.residentMemory) / totalMemKB) * 100.0F;
        }
    }

    std::vector<ProcessInfo> results;
    results.reserve(processMap.size());
    for (const auto& pair : processMap) {
        results.push_back(pair.second);
    }

    return results;
}

std::expected<std::vector<ProcessCpuUsage>, AnalyzerErrorDetail> ProcessAnalyzer::getAllProcessesCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshotResult = getBasicSnapshot(*this);
    if (!initialSnapshotResult) {
        return std::unexpected(initialSnapshotResult.error());
    }
    auto initialSnapshot = *initialSnapshotResult;

    auto initialTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!initialTotalSystemTicksResult) {
        return std::unexpected(initialTotalSystemTicksResult.error());
    }
    long long initialTotalSystemTicks = *initialTotalSystemTicksResult;

    std::this_thread::sleep_for(durationMs);

    auto finalSnapshotResult = getBasicSnapshot(*this);
     if (!finalSnapshotResult) {
        return std::unexpected(finalSnapshotResult.error());
    }
    auto finalSnapshot = *finalSnapshotResult;

    auto finalTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!finalTotalSystemTicksResult) {
        return std::unexpected(finalTotalSystemTicksResult.error());
    }
    long long finalTotalSystemTicks = *finalTotalSystemTicksResult;

    long long totalSystemTicksDelta = finalTotalSystemTicks - initialTotalSystemTicks;
    std::vector<ProcessCpuUsage> results;
    std::map<int, ProcessInfo> finalSnapshotMap;
    for(const auto& info : finalSnapshot) {
        finalSnapshotMap[info.pid] = info;
    }

    for (const auto& initialInfo : initialSnapshot) {
        auto it = finalSnapshotMap.find(initialInfo.pid);
        if (it != finalSnapshotMap.end()) {
            const auto& finalInfo = it->second;
            long long processCpuTicksDelta = (finalInfo.cpuUserTimeTicks + finalInfo.cpuKernelTimeTicks) - (initialInfo.cpuUserTimeTicks + initialInfo.cpuKernelTimeTicks);
            
            ProcessCpuUsage usage;
            usage.pid = initialInfo.pid;
            usage.name = finalInfo.name;
            if (totalSystemTicksDelta > 0) {
                usage.cpuPercentage = 100.0 * static_cast<double>(processCpuTicksDelta) / static_cast<double>(totalSystemTicksDelta);
            } else {
                usage.cpuPercentage = 0.0;
            }
            results.push_back(usage);
        }
    }

    return results;
}

std::expected<SystemMemoryInfo, AnalyzerErrorDetail> ProcessAnalyzer::getSystemMemoryInfo() const {
    std::string meminfoPath = std::string(procPath) + "/meminfo";
    auto contentOpt = utils::readTextFile(meminfoPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read /proc/meminfo",
            .systemErrno = errno
        });
    }

    SystemMemoryInfo memInfo;
    std::stringstream ss(*contentOpt);
    std::string line;
    while (std::getline(ss, line)) {
        std::string key;
        unsigned long value;
        std::stringstream lineSs(line);
        lineSs >> key >> value;
        if(key.empty()) continue;
        key.pop_back(); 

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

std::expected<SystemLoadAverage, AnalyzerErrorDetail> ProcessAnalyzer::getSystemLoadAverage() const {
    std::string loadavgPath = std::string(procPath) + "/loadavg";
    auto contentOpt = utils::readTextFile(loadavgPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read /proc/loadavg",
            .systemErrno = errno
        });
    }

    SystemLoadAverage loadAvg;
    std::stringstream ss(*contentOpt);
    ss >> loadAvg.oneMin >> loadAvg.fiveMin >> loadAvg.fifteenMin;
    return loadAvg;
}

std::expected<SystemCpuStats, AnalyzerErrorDetail> ProcessAnalyzer::getSystemCpuStats() const {
    std::string statPath = std::string(procPath) + "/stat";
    auto contentOpt = utils::readTextFile(statPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read /proc/stat",
            .systemErrno = errno
        });
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line);

    if (line.starts_with("cpu ")) {
        SystemCpuStats stats;
        std::stringstream lineSs(line);
        std::string cpuLabel;
        lineSs >> cpuLabel >> stats.user >> stats.nice >> stats.system >> stats.idle 
                >> stats.iowait >> stats.irq >> stats.softirq >> stats.steal;
        return stats;
    }

    return std::unexpected(AnalyzerErrorDetail{
        .code = AnalyzerError::parsingError,
        .message = "Could not find 'cpu' line in /proc/stat",
        .systemErrno = std::nullopt
    });
}


std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> ProcessAnalyzer::findProcesses(const ProcessPredicate& predicate) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return std::unexpected(allProcessesResult.error());
    }

    std::vector<ProcessInfo> filtered;
    for (const auto& info : *allProcessesResult) {
        if (predicate(info)) {
            filtered.push_back(info);
        }
    }
    return filtered;
}

std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> ProcessAnalyzer::queryProcesses(
    const ProcessFilter& filter,
    ProcessSortField sortBy,
    SortOrder sortOrder
) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return std::unexpected(allProcessesResult.error());
    }
    auto allProcesses = *allProcessesResult;

    std::vector<ProcessInfo> filteredProcesses;

    std::ranges::copy_if(allProcesses, std::back_inserter(filteredProcesses),
        [&](const ProcessInfo& process) {
            return matchesFilter(process, filter);
        });

    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
        bool less = false; 
        switch (sortBy) {
            case ProcessSortField::pid: less = a.pid < b.pid; break;
            case ProcessSortField::ppid: less = a.ppid < b.ppid; break;
            case ProcessSortField::uid: less = a.uid < b.uid; break;
            case ProcessSortField::user: less = a.username < b.username; break;
            case ProcessSortField::name: less = a.name < b.name; break;
            case ProcessSortField::state: less = a.state < b.state; break;
            case ProcessSortField::rss: less = a.residentMemory < b.residentMemory; break;
            case ProcessSortField::vmsize: less = a.virtualMemory < b.virtualMemory; break;
            case ProcessSortField::threads: less = a.threadCount < b.threadCount; break;
            case ProcessSortField::startTime: less = a.startTimeTicks < b.startTimeTicks; break;
            case ProcessSortField::executablePath: less = a.executablePath < b.executablePath; break;
            case ProcessSortField::cmdline: less = a.cmdline < b.cmdline; break;
            case ProcessSortField::cpuTime:
                less = (a.cpuUserTimeTicks + a.cpuKernelTimeTicks) < (b.cpuUserTimeTicks + b.cpuKernelTimeTicks);
                break;
            case ProcessSortField::cwd: less = a.currentWorkingDirectory < b.currentWorkingDirectory; break;
            case ProcessSortField::cpuUserTime: less = a.cpuUserTimeTicks < b.cpuUserTimeTicks; break;
            case ProcessSortField::cpuKernelTime: less = a.cpuKernelTimeTicks < b.cpuKernelTimeTicks; break;
            case ProcessSortField::ioReadBytes: less = a.ioReadBytes < b.ioReadBytes; break;
            case ProcessSortField::ioWriteBytes: less = a.ioWriteBytes < b.ioWriteBytes; break;
            case ProcessSortField::priority: less = a.priority < b.priority; break;
            case ProcessSortField::cpuUsage: less = a.cpuUsage < b.cpuUsage; break;          
            case ProcessSortField::memoryPercentage: less = a.memoryPercentage < b.memoryPercentage; break;
        }

        return (sortOrder == SortOrder::asc) ? less : !less;
    });

    return filteredProcesses;
}

std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> ProcessAnalyzer::getChildProcesses(int pid) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return std::unexpected(allProcessesResult.error());
    }
    std::vector<ProcessInfo> children;
    for (const auto& process : *allProcessesResult) {
        if (process.ppid == pid) {
            children.push_back(process);
        }
    }
    return children;
}

std::expected<ProcessCpuUsage, AnalyzerErrorDetail> ProcessAnalyzer::getProcessCpuUsage(int pid, std::chrono::milliseconds durationMs) const {
    auto initialDetailsResult = getProcessDetails(pid);
    if (!initialDetailsResult) {
        return std::unexpected(initialDetailsResult.error());
    }
    auto initialDetails = *initialDetailsResult;

    auto initialTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!initialTotalSystemTicksResult) {
        return std::unexpected(initialTotalSystemTicksResult.error());
    }
    long long initialTotalSystemTicks = *initialTotalSystemTicksResult;


    std::this_thread::sleep_for(durationMs);

    auto finalDetailsResult = getProcessDetails(pid);
    if (!finalDetailsResult) {
        return ProcessCpuUsage{.pid=pid, .name=initialDetails.name, .cpuPercentage=0.0};
    }
    // const auto& finalDetails = *finalDetailsResult; // Fixed error here (removed auto copy)
    const auto& finalDetails = *finalDetailsResult;

    auto finalTotalSystemTicksResult = getTotalSystemCpuTimeTicks(procPath);
    if (!finalTotalSystemTicksResult) {
        return std::unexpected(finalTotalSystemTicksResult.error());
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

std::expected<std::vector<std::string>, AnalyzerErrorDetail> ProcessAnalyzer::getProcessEnvironment(int pid) const {
    std::vector<std::string> env;
    std::string environPath = std::string(procPath) + "/" + std::to_string(pid) + "/environ";

    auto environContentOpt = utils::readTextFile(environPath);
    if (!environContentOpt) {
        std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(AnalyzerErrorDetail{
                .code = AnalyzerError::processNotFound,
                .message = "Process with PID " + std::to_string(pid) + " not found.",
                .systemErrno = std::nullopt
            });
        }
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::permissionDenied,
            .message = "Could not read environment for PID " + std::to_string(pid),
            .systemErrno = errno
        });
    }

    std::string_view content = *environContentOpt;
    size_t start = 0;
    while(start < content.size()) {
        size_t end = content.find('\0', start);
        if (end == std::string_view::npos) break;
        env.emplace_back(content.substr(start, end - start));
        start = end + 1;
    }
    return env;
}

std::expected<ProcessInfo, AnalyzerErrorDetail> ProcessAnalyzer::getParentProcess(int pid) const {
    auto detailsResult = getProcessDetails(pid);
    if (!detailsResult) {
        return std::unexpected(detailsResult.error());
    }
    if (detailsResult->ppid == 0) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::processNotFound,
            .message = "Process " + std::to_string(pid) + " has no parent (it might be the init process).",
            .systemErrno = std::nullopt
        });
    }
    return getProcessDetails(detailsResult->ppid);
}

std::expected<std::vector<ProcessInfo>, AnalyzerErrorDetail> ProcessAnalyzer::getAllDescendantProcesses(int pid) const {
    auto allProcessesResult = snapshot();
    if (!allProcessesResult) {
        return std::unexpected(allProcessesResult.error());
    }
    auto allProcesses = *allProcessesResult;

    std::vector<ProcessInfo> descendants;
    std::vector<int> toVisit {pid};
    std::map<int, std::vector<ProcessInfo>> parentToChildren;
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

std::expected<long, AnalyzerErrorDetail> ProcessAnalyzer::getSystemClockTicksPerSecond() {
    long ticks = sysconf(_SC_CLK_TCK);
    if (ticks < 0) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::systemError,
            .message = "Failed to get system clock ticks per second.",
            .systemErrno = errno
        });
    }
    return ticks;
}


std::expected<void, AnalyzerErrorDetail> ProcessAnalyzer::sendSignal(int pid, ProcessSignal signal) {
    if (::kill(pid, static_cast<int>(signal)) == 0) {
        return {};
    }

    AnalyzerError errCode;
    std::string errMsg;
    switch (errno) {
        case ESRCH:
            errCode = AnalyzerError::processNotFound;
            errMsg = "Process with PID " + std::to_string(pid) + " not found.";
            break;
        case EPERM:
            errCode = AnalyzerError::permissionDenied;
            errMsg = "Permission denied to send signal to PID " + std::to_string(pid) + ".";
            break;
        default:
            errCode = AnalyzerError::systemError;
            errMsg = "Failed to send signal to PID " + std::to_string(pid) + ".";
            break;
    }
    return std::unexpected(AnalyzerErrorDetail{
        .code = errCode,
        .message = errMsg,
        .systemErrno = errno
    });
}

std::expected<std::vector<ThreadInfo>, AnalyzerErrorDetail> ProcessAnalyzer::getProcessThreads(int pid) const {
    std::vector<ThreadInfo> threads;
    std::string taskPath = std::string(procPath) + "/" + std::to_string(pid) + "/task";

    if (!fs::exists(taskPath) || !fs::is_directory(taskPath)) {
        std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(AnalyzerErrorDetail{
                .code = AnalyzerError::processNotFound,
                .message = "Process with PID " + std::to_string(pid) + " not found.",
                .systemErrno = std::nullopt
            });
        }
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::permissionDenied,
            .message = "Cannot access task directory for PID " + std::to_string(pid),
            .systemErrno = std::nullopt
        });
    }

    try {
        for (const auto& entry : fs::directory_iterator(taskPath)) {
            if (entry.is_directory()) {
                std::string tidStr = entry.path().filename().string();
                if (utils::isInteger(tidStr)) {
                    int tid = std::stoi(tidStr);
                    ThreadInfo thread;
                    thread.tid = tid;

                    std::string commPath = entry.path().string() + "/comm";
                    if (auto commContent = utils::readTextFile(commPath)) {
                        thread.name = utils::trim(*commContent);
                    }

                    std::string statPath = entry.path().string() + "/stat";
                    if (auto statContent = utils::readTextFile(statPath)) {
                        std::stringstream ss(*statContent);
                        std::string comm;
                        std::string dummy;
                        char stateChar;
                        ss >> dummy >> comm >> stateChar; 
                        thread.state = stateChar;

                        for (int i = 0; i < statFieldsToSkipBeforeThreadUtime; ++i) ss >> dummy;
                        ss >> thread.cpuUserTimeTicks >> thread.cpuKernelTimeTicks;
                    }
                    threads.push_back(thread);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::permissionDenied,
            .message = "Failed to iterate task directory for PID " + std::to_string(pid) + ": " + e.what(),
            .systemErrno = e.code().value()
        });
    }
    return threads;
}

std::expected<std::vector<MountPointInfo>, AnalyzerErrorDetail> ProcessAnalyzer::getSystemDiskUsage() const {
    std::vector<MountPointInfo> mounts;
    std::string mountsPath = std::string(procPath) + "/mounts";
    auto contentOpt = utils::readTextFile(mountsPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read " + mountsPath,
            .systemErrno = errno
        });
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        std::stringstream lineSs(line);
        std::string device;
        std::string mountPoint;
        std::string filesystemType;
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

std::expected<SystemInfo, AnalyzerErrorDetail> ProcessAnalyzer::getSystemInfo() const {
    SystemInfo sysInfo;

    std::string uptimePath = std::string(procPath) + "/uptime";
    if (auto content = utils::readTextFile(uptimePath)) {
        double uptimeSecs;
        std::stringstream ss(*content);
        ss >> uptimeSecs;
        sysInfo.uptime = std::chrono::seconds(static_cast<long long>(uptimeSecs));
    } else {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read " + uptimePath,
            .systemErrno = errno
        });
    }

    std::string versionPath = std::string(procPath) + "/version";
    if (auto content = utils::readTextFile(versionPath)) {
        sysInfo.kernelVersion = utils::trim(*content);
    } else {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::fileNotFound,
            .message = "Could not read " + versionPath,
            .systemErrno = errno
        });
    }

    if (auto content = utils::readTextFile("/etc/os-release")) {
        std::stringstream ss(*content);
        std::string line;
        while(std::getline(ss, line)) {
            if (line.starts_with("PRETTY_NAME=")) {
                sysInfo.osName = line.substr(line.find('=') + 1);
                std::erase(sysInfo.osName, '"');
                break;
            }
        }
    } else {
        sysInfo.osName = "Unknown";
    }

    constexpr size_t kMaxHostnameLen = 256;
    std::array<char, kMaxHostnameLen> hostname{};
    if (gethostname(hostname.data(), hostname.size()) == 0) {
        sysInfo.hostname = hostname.data();
    } else {
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::systemError,
            .message = "Could not get hostname.",
            .systemErrno = errno
        });
    }
    
    return sysInfo;
}

std::expected<SystemCpuUsage, AnalyzerErrorDetail> ProcessAnalyzer::getSystemCpuUsage(std::chrono::milliseconds durationMs) const {
    auto initialStatsResult = getSystemCpuStats();
    if (!initialStatsResult) return std::unexpected(initialStatsResult.error());
    auto initialStats = *initialStatsResult;
    
    std::this_thread::sleep_for(durationMs);

    auto finalStatsResult = getSystemCpuStats();
    if (!finalStatsResult) return std::unexpected(finalStatsResult.error());
    auto finalStats = *finalStatsResult;

    unsigned long long initialTotal = initialStats.user + initialStats.nice + initialStats.system + initialStats.idle + initialStats.iowait + initialStats.irq + initialStats.softirq + initialStats.steal;
    unsigned long long finalTotal = finalStats.user + finalStats.nice + finalStats.system + finalStats.idle + finalStats.iowait + finalStats.irq + finalStats.softirq + finalStats.steal;

    unsigned long long totalDelta = finalTotal - initialTotal;
    if (totalDelta == 0) return SystemCpuUsage{0.0};

    unsigned long long idleDelta = finalStats.idle - initialStats.idle;
    
    double usage = 100.0 * (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta));
    return SystemCpuUsage{usage};
}

std::expected<ProcessDiskIoUsage, AnalyzerErrorDetail> ProcessAnalyzer::getProcessDiskIoUsage(int pid, std::chrono::milliseconds durationMs) const {
    auto initialDetailsResult = getProcessDetails(pid);
    if (!initialDetailsResult) return std::unexpected(initialDetailsResult.error());
    auto initialDetails = *initialDetailsResult;

    std::this_thread::sleep_for(durationMs);
    
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
    double durationSec = static_cast<double>(durationMs.count()) / kMSInSecond;

    return ProcessDiskIoUsage {
        .pid = pid,
        .name = finalDetails.name,
        .readBytesPerSecond = durationSec > 0 ? readDelta / durationSec : 0.0,
        .writeBytesPerSecond = durationSec > 0 ? writeDelta / durationSec : 0.0
    };
}

std::expected<std::vector<ProcessDiskIoUsage>, AnalyzerErrorDetail> ProcessAnalyzer::getAllProcessesDiskIoUsage(std::chrono::milliseconds durationMs) const {
    auto initialSnapshotResult = snapshot();
    if (!initialSnapshotResult) return std::unexpected(initialSnapshotResult.error());
    auto initialSnapshot = *initialSnapshotResult;

    std::this_thread::sleep_for(durationMs);

    auto finalSnapshotResult = snapshot();
    if (!finalSnapshotResult) return std::unexpected(finalSnapshotResult.error());
    auto finalSnapshot = *finalSnapshotResult;

    std::map<int, ProcessInfo> finalSnapshotMap;
    for(const auto& info : finalSnapshot) {
        finalSnapshotMap[info.pid] = info;
    }

    std::vector<ProcessDiskIoUsage> results;
    double durationSec = static_cast<double>(durationMs.count()) / kMSInSecond;
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

std::expected<std::vector<OpenFileDescriptorInfo>, AnalyzerErrorDetail> ProcessAnalyzer::getProcessOpenFileDetails(pid_t pid) const {
    std::vector<OpenFileDescriptorInfo> openFds;
    std::string fdPath = std::string(procPath) + "/" + std::to_string(pid) + "/fd";

    DIR* dir = opendir(fdPath.c_str());
    if (!dir) {
        std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(AnalyzerErrorDetail{
                .code = AnalyzerError::processNotFound,
                .message = "Process with PID " + std::to_string(pid) + " not found.",
                .systemErrno = std::nullopt
            });
        }
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::permissionDenied,
            .message = "Cannot access file descriptor directory for PID " + std::to_string(pid),
            .systemErrno = errno
        });
    }
    int dirFd = dirfd(dir);

    try {
        for (const auto& entry : fs::directory_iterator(fdPath)) {
            if (!entry.is_symlink()) continue;

            std::string fdStr = entry.path().filename().string();
            if (!utils::isInteger(fdStr)) continue;

            int fd = std::stoi(fdStr);
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
        closedir(dir);
        return std::unexpected(AnalyzerErrorDetail{
            .code = AnalyzerError::permissionDenied,
            .message = "Failed to iterate open files directory for PID " + std::to_string(pid) + ": " + e.what(),
            .systemErrno = e.code().value()
        });
    }

    closedir(dir);
    return openFds;
}

std::expected<std::vector<NetworkConnection>, AnalyzerErrorDetail> ProcessAnalyzer::getNetworkConnections(int pid) const {
    auto fdsResult = getProcessOpenFileDetails(pid);
    if (!fdsResult) {
        return std::unexpected(fdsResult.error());
    }

    std::set<int> socketInodes;
    constexpr int kSocketInodePrefixLen = 8; // "socket:["
    constexpr int kSocketInodeSuffixLen = 1; // "]"

    for (const auto& fdInfo : *fdsResult) {
        if (fdInfo.type == OpenFileType::Socket) {
            if (fdInfo.path.starts_with("socket:[")) {
                std::string inodeStr = fdInfo.path.substr(kSocketInodePrefixLen, fdInfo.path.length() - kSocketInodePrefixLen - kSocketInodeSuffixLen);
                if(utils::isInteger(inodeStr)){
                    socketInodes.insert(std::stoi(inodeStr));
                }
            }
        }
    }

    if(socketInodes.empty()) return std::vector<NetworkConnection>{};

    std::vector<InternalNetworkConnection> allInternalConnections;
    for (const auto& conn : parseNetFileHelper(std::string(procPath) + "/net/tcp", "TCP")) { allInternalConnections.push_back(conn); }
    for (const auto& conn : parseNetFileHelper(std::string(procPath) + "/net/tcp6", "TCP6")) { allInternalConnections.push_back(conn); }
    for (const auto& conn : parseNetFileHelper(std::string(procPath) + "/net/udp", "UDP")) { allInternalConnections.push_back(conn); }
    for (const auto& conn : parseNetFileHelper(std::string(procPath) + "/net/udp6", "UDP6")) { allInternalConnections.push_back(conn); }
    
    std::vector<NetworkConnection> connections;
    for(const auto& internalConn : allInternalConnections){
        if(socketInodes.contains(internalConn.inode)){
            connections.push_back(internalConn.baseConn);
        }
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
            std::string filename = entry.path().filename().string();
            if (utils::isInteger(filename)) {
                co_yield std::stoi(filename);
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
    std::vector<ProcessInfo> filteredProcesses;
    for (const auto& process : streamProcesses()) {
         if (matchesFilter(process, filter)) {
             filteredProcesses.push_back(process);
         }
    }
    
    std::ranges::sort(filteredProcesses,
        [&](const ProcessInfo& a, const ProcessInfo& b) {
        bool less = false;
        switch (sortBy) {
            case ProcessSortField::pid: less = a.pid < b.pid; break;
            case ProcessSortField::ppid: less = a.ppid < b.ppid; break;
            case ProcessSortField::uid: less = a.uid < b.uid; break;
            case ProcessSortField::user: less = a.username < b.username; break;
            case ProcessSortField::name: less = a.name < b.name; break;
            case ProcessSortField::state: less = a.state < b.state; break;
            case ProcessSortField::rss: less = a.residentMemory < b.residentMemory; break;
            case ProcessSortField::vmsize: less = a.virtualMemory < b.virtualMemory; break;
            case ProcessSortField::threads: less = a.threadCount < b.threadCount; break;
            case ProcessSortField::startTime: less = a.startTimeTicks < b.startTimeTicks; break;
            case ProcessSortField::executablePath: less = a.executablePath < b.executablePath; break;
            case ProcessSortField::cmdline: less = a.cmdline < b.cmdline; break;
            case ProcessSortField::cpuTime:
                less = (a.cpuUserTimeTicks + a.cpuKernelTimeTicks) < (b.cpuUserTimeTicks + b.cpuKernelTimeTicks);
                break;
            case ProcessSortField::cwd: less = a.currentWorkingDirectory < b.currentWorkingDirectory; break;
            case ProcessSortField::cpuUserTime: less = a.cpuUserTimeTicks < b.cpuUserTimeTicks; break;
            case ProcessSortField::cpuKernelTime: less = a.cpuKernelTimeTicks < b.cpuKernelTimeTicks; break;
            case ProcessSortField::ioReadBytes: less = a.ioReadBytes < b.ioReadBytes; break;
            case ProcessSortField::ioWriteBytes: less = a.ioWriteBytes < b.ioWriteBytes; break;
            case ProcessSortField::priority: less = a.priority < b.priority; break;
            case ProcessSortField::cpuUsage: less = a.cpuUsage < b.cpuUsage; break;          
            case ProcessSortField::memoryPercentage: less = a.memoryPercentage < b.memoryPercentage; break; 
            default: less = a.pid < b.pid; break;
        }
        return (sortOrder == SortOrder::asc) ? less : !less;
    });

    for(auto& process : filteredProcesses){
        co_yield process;
    }
}

std::expected<std::vector<MemoryMapInfo>, AnalyzerErrorDetail> ProcessAnalyzer::getProcessMemoryMaps(int pid) const {
    std::vector<MemoryMapInfo> maps;
    std::string mapsPath = std::string(procPath) + "/" + std::to_string(pid) + "/maps";

    auto contentOpt = utils::readTextFile(mapsPath);
    if (!contentOpt) {
        std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(AnalyzerErrorDetail{.code = AnalyzerError::processNotFound, .message = "Process with PID " + std::to_string(pid) + " not found.", .systemErrno = std::nullopt});
        }
        return std::unexpected(AnalyzerErrorDetail{.code = AnalyzerError::permissionDenied, .message = "Could not read memory maps for PID " + std::to_string(pid), .systemErrno = errno});
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    while (std::getline(ss, line)) {
        MemoryMapInfo mapInfo;
        char dash;
        
        std::stringstream lineSs(line);
        lineSs >> std::hex >> mapInfo.startAddress >> dash >> mapInfo.endAddress >> std::dec;
        lineSs >> mapInfo.permissions;
        lineSs >> std::hex >> mapInfo.offset >> std::dec;
        lineSs >> mapInfo.device;
        lineSs >> mapInfo.inode;
        
        std::string pathname;
        if (std::getline(lineSs, pathname)) {
             mapInfo.pathname = utils::trim(pathname);
        } else {
             mapInfo.pathname = "";
        }

        maps.push_back(mapInfo);
    }

    return maps;
}

std::expected<ResourceLimitInfo, AnalyzerErrorDetail> ProcessAnalyzer::getProcessResourceLimits(int pid) const {
    ResourceLimitInfo limitInfo;
    std::string limitsPath = std::string(procPath) + "/" + std::to_string(pid) + "/limits";

    auto contentOpt = utils::readTextFile(limitsPath);
    if (!contentOpt) {
        std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(AnalyzerErrorDetail{.code = AnalyzerError::processNotFound, .message = "Process with PID " + std::to_string(pid) + " not found.", .systemErrno = std::nullopt});
        }
        return std::unexpected(AnalyzerErrorDetail{.code = AnalyzerError::fileNotFound, .message = "Could not read limits for PID " + std::to_string(pid), .systemErrno = errno });
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line); 

    std::string header = line;
    size_t softLimitPos = header.find("Soft Limit");
    size_t hardLimitPos = header.find("Hard Limit");
    size_t unitsPos = header.find("Units");

    if (softLimitPos == std::string::npos || hardLimitPos == std::string::npos || unitsPos == std::string::npos) {
        return std::unexpected(AnalyzerErrorDetail{.code = AnalyzerError::parsingError, .message = "Could not parse limits file header for PID " + std::to_string(pid), .systemErrno = std::nullopt});
    }

    while (std::getline(ss, line)) {
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

std::expected<CgroupInfo, AnalyzerErrorDetail> ProcessAnalyzer::getProcessCgroupInfo(int pid) const {
    CgroupInfo cgroupInfo;
    std::string cgroupPath = std::string(procPath) + "/" + std::to_string(pid) + "/cgroup";

    auto contentOpt = utils::readTextFile(cgroupPath);
     if (!contentOpt) {
        std::string pidPath = std::string(procPath) + "/" + std::to_string(pid);
        if (!fs::exists(pidPath)) {
            return std::unexpected(AnalyzerErrorDetail{.code = AnalyzerError::processNotFound, .message = "Process with PID " + std::to_string(pid) + " not found.", .systemErrno = std::nullopt});
        }
        return std::unexpected(AnalyzerErrorDetail{ .code = AnalyzerError::fileNotFound, .message = "Could not read cgroup for PID " + std::to_string(pid), .systemErrno = errno });
    }
    
    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        CgroupEntry entry;
        char colon;
        std::stringstream lineSs(line);
        lineSs >> entry.id >> colon;
        std::getline(lineSs, entry.controllers, ':');
        std::getline(lineSs, entry.path);
        cgroupInfo.entries.push_back(entry);
    }

    return cgroupInfo;
}

// Implementations for NEW API methods
std::expected<void, AnalyzerErrorDetail> ProcessAnalyzer::setProcessNiceness(int pid, int niceness) {
    if (setpriority(PRIO_PROCESS, pid, niceness) == 0) {
        return {};
    }
    return std::unexpected(AnalyzerErrorDetail{
        .code = (errno == EACCES || errno == EPERM) ? AnalyzerError::permissionDenied : AnalyzerError::systemError,
        .message = "Failed to set process niceness.",
        .systemErrno = errno
    });
}

std::expected<void, AnalyzerErrorDetail> ProcessAnalyzer::setProcessCpuAffinity(int pid, const CpuSet& affinity) {
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int cpu : affinity.cpus) {
        CPU_SET(cpu, &set);
    }
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &set) == 0) {
        return {};
    }
    return std::unexpected(AnalyzerErrorDetail{
        .code = (errno == EACCES || errno == EPERM) ? AnalyzerError::permissionDenied : AnalyzerError::systemError,
        .message = "Failed to set process CPU affinity.",
        .systemErrno = errno
    });
}

std::expected<PerCpuUsage, AnalyzerErrorDetail> ProcessAnalyzer::getPerCpuUsage(std::chrono::milliseconds durationMs) const {
    struct CpuSnapshot {
        unsigned long long total;
        unsigned long long idle;
    };
    
    auto getSnapshots = [&](const std::string& path) -> std::vector<CpuSnapshot> {
        std::vector<CpuSnapshot> snapshots;
        auto contentOpt = utils::readTextFile(path);
        if (!contentOpt) return snapshots;

        std::stringstream ss(*contentOpt);
        std::string line;
        while(std::getline(ss, line)) {
             if (line.starts_with("cpu")) {
                std::stringstream lineSs(line);
                std::string label;
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

    std::string statPath = std::string(procPath) + "/stat";
    auto initialSnapshots = getSnapshots(statPath);
    if (initialSnapshots.empty()) return std::unexpected(AnalyzerErrorDetail{.code=AnalyzerError::parsingError, .message="Failed to parse /proc/stat", .systemErrno = std::nullopt});

    std::this_thread::sleep_for(durationMs);

    auto finalSnapshots = getSnapshots(statPath);
    if (finalSnapshots.empty()) return std::unexpected(AnalyzerErrorDetail{.code=AnalyzerError::parsingError, .message="Failed to parse /proc/stat", .systemErrno = std::nullopt});

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

std::expected<std::vector<DiskIoDeviceStats>, AnalyzerErrorDetail> ProcessAnalyzer::getSystemDiskIoStats() const {
    std::vector<DiskIoDeviceStats> stats;
    std::string diskStatsPath = std::string(procPath) + "/diskstats";
    auto contentOpt = utils::readTextFile(diskStatsPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{.code=AnalyzerError::fileNotFound, .message="Could not read /proc/diskstats", .systemErrno = errno});
    }
    
    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        std::stringstream lineSs(line);
        int major;
        int minor;
        std::string deviceName;
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

std::expected<std::vector<NetworkInterfaceStats>, AnalyzerErrorDetail> ProcessAnalyzer::getNetworkInterfaceStats() const {
    std::vector<NetworkInterfaceStats> stats;
    std::string netDevPath = std::string(procPath) + "/net/dev";
    auto contentOpt = utils::readTextFile(netDevPath);
    if (!contentOpt) {
        return std::unexpected(AnalyzerErrorDetail{.code=AnalyzerError::fileNotFound, .message="Could not read /proc/net/dev", .systemErrno = errno});
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line); // header 1
    std::getline(ss, line); // header 2

    while(std::getline(ss, line)) {
        std::stringstream lineSs(line);
        std::string interfaceName;
        std::getline(lineSs, interfaceName, ':');
        interfaceName = utils::trim(interfaceName);
        
        NetworkInterfaceStats stat;
        stat.interfaceName = interfaceName;
        
        lineSs >> stat.rxBytes >> stat.rxPackets >> stat.rxErrors >> stat.rxDropped
               >> stat.rxDropped >> stat.rxDropped >> stat.rxDropped >> stat.rxDropped // skipping fifo, frame, compressed, multicast
               >> stat.txBytes >> stat.txPackets >> stat.txErrors >> stat.txDropped;
               
        stats.push_back(stat);
    }
    return stats;
}

std::expected<SystemActivityStats, AnalyzerErrorDetail> ProcessAnalyzer::getSystemActivityStats() const {
    SystemActivityStats stats;
    stats.interruptsTotal = 0;
    stats.contextSwitches = 0;
    stats.processesForked = 0;

    std::string statPath = std::string(procPath) + "/stat";
    auto contentOpt = utils::readTextFile(statPath);
    if (!contentOpt) {
         return std::unexpected(AnalyzerErrorDetail{.code=AnalyzerError::fileNotFound, .message="Could not read /proc/stat", .systemErrno = errno});
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    while(std::getline(ss, line)) {
        if (line.starts_with("intr ")) {
            std::stringstream lineSs(line);
            std::string label;
            lineSs >> label >> stats.interruptsTotal;
        } else if (line.starts_with("ctxt ")) {
             std::stringstream lineSs(line);
             std::string label;
             lineSs >> label >> stats.contextSwitches;
        } else if (line.starts_with("processes ")) {
             std::stringstream lineSs(line);
             std::string label;
             lineSs >> label >> stats.processesForked;
        }
    }
    return stats;
}

