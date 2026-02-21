// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef TESTING_FRAMEWORK_H
#define TESTING_FRAMEWORK_H

#include <string>
#include <filesystem>
#include <vector>
#include <map>
#include <utility> // For std::pair

namespace fs = std::filesystem;

class MockProc; // Forward declaration

class ProcessBuilder; // Forward declaration

class MockProc {
public:
    explicit MockProc(const std::string& basePath = "mock_proc");
    ~MockProc();

    [[nodiscard]] std::string getPath() const;

    // Basic file creation
    void createProcFile(int pid, const std::string& filename, const std::string& content);

    void createFileAt(const std::filesystem::path& relativePath, const std::string& content);

    void createDirectoryAt(const std::filesystem::path& relativePath);

    void createSymlinkAt(const std::filesystem::path& relativeLinkPath, const std::filesystem::path& targetPath);
    void createCmdline(int pid, const std::vector<std::string>& args);
    void createStatus(int pid, const std::map<std::string, std::string>& data);
    void createEnviron(int pid, const std::map<std::string, std::string>& envVars);
    void createFdDir(int pid, const std::vector<std::pair<int, std::string>>& fds);


    // Iteration 8



    void createComm(int pid, const std::string& commName);

    // Structs for complex files
    struct ProcStatData {
        int pid = 0;
        std::string comm;
        char state = 'R';
        int ppid = 0;
        int pgrp = 0;
        int session = 0;
        int tty_nr = 0;
        int tpgid = 0;
        unsigned int flags = 0;
        unsigned long minflt = 0, cminflt = 0, majflt = 0, cmajflt = 0;
        unsigned long utime = 0, stime = 0;
        long cutime = 0, cstime = 0;
        long priority = 20, nice = 0;
        long num_threads = 1;
        long itrealvalue = 0;
        unsigned long long starttime = 0;
        unsigned long vsize = 0;
        long rss = 0;
        unsigned long rsslim = 0;
        unsigned long startcode = 0, endcode = 0, startstack = 0;
        unsigned long kstkesp = 0, kstkeip = 0;
        unsigned long signal = 0, blocked = 0, sigignore = 0, sigcatch = 0;
        unsigned long wchan = 0;
        unsigned long nswap = 0, cnswap = 0;
        int exit_signal = 17, processor = 0;
        unsigned int rt_priority = 0, policy = 0;
        unsigned long long delayacct_blkio_ticks = 0;
        unsigned long guest_time = 0;
        long cguest_time = 0;
        unsigned long start_data = 0, end_data = 0, start_brk = 0;
        unsigned long arg_start = 0, arg_end = 0, env_start = 0, env_end = 0;
        int exit_code = 0;

        [[nodiscard]] std::string toString() const;
    };
    
    struct ProcIoStats {
        unsigned long rchar = 0;
        unsigned long wchar = 0;
        unsigned long syscr = 0;
        unsigned long syscw = 0;
        unsigned long readBytes = 0;
        unsigned long writeBytes = 0;
        unsigned long cancelledWriteBytes = 0;
        [[nodiscard]] std::string toString() const;
    };

    struct ProcMapEntry {
        std::string addressRange; // e.g., "7f000000-7f010000"
        std::string perms;        // e.g., "r-xp"
        unsigned long offset = 0;
        std::string dev = "00:00";
        unsigned long inode = 0;
        fs::path pathname;
        [[nodiscard]] std::string toString() const;
    };

    struct MeminfoData {
        unsigned long memTotalKb = 0;
        unsigned long memFreeKb = 0;
        unsigned long memAvailableKb = 0;
        unsigned long buffersKb = 0;
        unsigned long cachedKb = 0;
        unsigned long swapTotalKb = 0;
        unsigned long swapFreeKb = 0;
        [[nodiscard]] std::string toString() const;
    };

    struct CpuinfoData {
        int processorId = 0;
        std::string vendorId = "GenuineIntel";
        std::string modelName = "Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz";
        float cpuMhz = 2600.000;
        int siblings = 12;
        int cpuCores = 6;
        [[nodiscard]] std::string toString() const;
    };

    // --- Iteration 12 Additions ---
    struct SystemStatData {
        unsigned long long user = 0, nice = 0, system = 0, idle = 0;
        unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0;
        unsigned long long guest = 0, guest_nice = 0;
        unsigned long long ctxt = 0, btime = 0, processes = 0;
        [[nodiscard]] std::string toString() const;
    };

    struct NetDevStats {
        std::string interface;
        unsigned long long rx_bytes = 0, rx_packets = 0, rx_errs = 0, rx_drop = 0, fifo_rx = 0, frame_rx = 0, compressed_rx = 0, multicast_rx = 0;
        unsigned long long tx_bytes = 0, tx_packets = 0, tx_errs = 0, tx_drop = 0, fifo_tx = 0, colls_tx = 0, carrier_tx = 0, compressed_tx = 0;
        [[nodiscard]] std::string toString() const;
    };

    struct AddThreadOptions {
        ProcStatData statData;
        bool populateDefaultStat = true;
        std::string name;
    };

    struct AddProcessOptions {
        std::string name;
        std::vector<std::string> cmdlineArgs;
        fs::path exePath;
        fs::path cwdPath;
        fs::path rootPath;
        std::map<std::string, std::string> statusExtra;
        std::map<std::string, std::string> environVars;
        std::vector<std::pair<int, std::string>> fds;
        ProcIoStats ioStats;
        ProcStatData statData;
        bool populateDefaultStat = false;
        
        // Iteration 12
        int ppid = -1;
        std::vector<ProcMapEntry> maps;
    };

    void createStat(int pid, const ProcStatData& data);
    void createIo(int pid, const ProcIoStats& stats);
    void createMaps(int pid, const std::vector<ProcMapEntry>& entries);
    void createMeminfo(const MeminfoData& data);
    void createCpuinfo(const std::vector<CpuinfoData>& cores);

    // Iteration 12 System Files
    void createSystemStat(const SystemStatData& data);
    void createSystemStat(const std::vector<SystemStatData>& perCpuData);
    void createUptime(double uptimeSeconds, double idleSeconds);
    void createVersion(const std::string& versionString);
    void createNetDev(const std::vector<NetDevStats>& devices);

    // Iteration 12 Thread Support
    void addThread(int parentPid, int threadId, const AddThreadOptions& options);

    // High level API
    void addProcess(int pid, const AddProcessOptions& options);
    [[nodiscard]] ProcessBuilder buildProcess(int pid);

private:
    fs::path root;
};

// ProcessBuilder Definition
class ProcessBuilder {
public:
    ProcessBuilder(MockProc& mockProc, int pid);

    ProcessBuilder& withName(const std::string& name);
    ProcessBuilder& withCmdline(const std::vector<std::string>& args);
    ProcessBuilder& withParent(int ppid);
    ProcessBuilder& withExe(const fs::path& exePath);
    ProcessBuilder& withCwd(const fs::path& cwdPath);
    ProcessBuilder& withRoot(const fs::path& rootPath);
    ProcessBuilder& withEnviron(const std::map<std::string, std::string>& envVars);
    ProcessBuilder& withFd(int fd, const std::string& target);
    ProcessBuilder& withIoStats(const MockProc::ProcIoStats& stats);
    ProcessBuilder& withStat(const MockProc::ProcStatData& data);
    ProcessBuilder& withStatusField(const std::string& key, const std::string& value);
    ProcessBuilder& withMap(const MockProc::ProcMapEntry& mapEntry);
    ProcessBuilder& withMaps(const std::vector<MockProc::ProcMapEntry>& mapEntries);
    
    void create();

private:
    MockProc& mockProc_;
    int pid_;
    MockProc::AddProcessOptions options_;
};

#endif // TESTING_FRAMEWORK_H
