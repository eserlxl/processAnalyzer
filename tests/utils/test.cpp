// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/test.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

constexpr int addressWidth = 16;
constexpr int flagsWidth = 8;
constexpr int addressPartWidth = 8;

MockProc::MockProc(const std::string& basePath) : root(basePath) {
    fs::create_directories(root);
}

MockProc::~MockProc() {
    std::error_code ec;
    // Try to remove directly first
    fs::remove_all(root, ec);
    
    if (ec) {
        // If failed (likely permission denied), try to restore permissions
        try {
            for (const auto& entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied)) {
                try {
                    fs::permissions(entry.path(), fs::perms::all, fs::perm_options::replace);
                } catch (...) {}
            }
            fs::permissions(root, fs::perms::all, fs::perm_options::replace);
        } catch (...) {}
        
        // Try removing again
        fs::remove_all(root, ec);
    }
}

std::string MockProc::getPath() const {
    return root.string();
}

void MockProc::createProcFile(int pid, const std::string& filename, const std::string& content) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    std::ofstream(pidPath / filename) << content;
}

void MockProc::createSymlink(int pid, const std::string& linkname, const std::string& target) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    fs::path linkPath = pidPath / linkname;
    try {
        if (fs::exists(linkPath)) fs::remove(linkPath);
        fs::create_symlink(target, linkPath);
    } catch (const std::exception& e) {
        (void)e;
    }
}

void MockProc::createPidDir(int pid) {
    fs::create_directory(root / std::to_string(pid));
}

void MockProc::createFile(const std::string& filename, const std::string& content) {
    std::ofstream(root / filename) << content;
}

void MockProc::removeFile(const std::string& filename) {
    fs::path filePath = root / filename;
    if (fs::exists(filePath)) {
        fs::remove(filePath);
    }
}

void MockProc::removeDirectoryAt(const std::filesystem::path& relativePath) {
    fs::path dirPath = root / relativePath;
    if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
        fs::remove_all(dirPath);
    }
}

void MockProc::createFileAt(const std::filesystem::path& relativePath, const std::string& content) {
    fs::path fullPath = root / relativePath;
    fs::create_directories(fullPath.parent_path());
    std::ofstream(fullPath) << content;
}

void MockProc::createDirectoryAt(const std::filesystem::path& relativePath) {
    fs::create_directories(root / relativePath);
}

void MockProc::createSymlinkAt(const std::filesystem::path& relativeLinkPath, const std::filesystem::path& targetPath) {
    fs::path fullLinkPath = root / relativeLinkPath;
    fs::create_directories(fullLinkPath.parent_path());
    try {
        if (fs::exists(fullLinkPath)) fs::remove(fullLinkPath);
        fs::create_symlink(targetPath, fullLinkPath);
    } catch (...) { // NOLINT
        // ignore
    }
}

void MockProc::createCmdline(int pid, const std::vector<std::string>& args) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    std::ofstream cmdlineFile(pidPath / "cmdline");
    for (size_t i = 0; i < args.size(); ++i) {
        cmdlineFile << args[i];
        if (i < args.size() - 1) {
            cmdlineFile << '\0';
        }
    }
}

void MockProc::createStatus(int pid, const std::map<std::string, std::string>& data) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    std::ofstream statusFile(pidPath / "status");
    for (const auto& pair : data) {
        statusFile << pair.first << ": " << pair.second << '\n';
    }
}

void MockProc::createEnviron(int pid, const std::map<std::string, std::string>& envVars) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    std::ofstream environFile(pidPath / "environ");
    bool first = true;
    for (const auto& pair : envVars) {
        if (!first) environFile << '\0';
        environFile << pair.first << "=" << pair.second;
        first = false;
    }
}

void MockProc::createFdDir(int pid, const std::vector<std::pair<int, std::string>>& fds) {
    fs::path fdPath = root / std::to_string(pid) / "fd";
    fs::create_directories(fdPath);
    for (const auto& fdPair : fds) {
        fs::create_symlink(fdPair.second, fdPath / std::to_string(fdPair.first));
    }
}

void MockProc::createProcFdLink(int pid, int fd, const std::string& target) {
    fs::path fdPath = root / std::to_string(pid) / "fd";
    fs::create_directories(fdPath);
    fs::create_symlink(target, fdPath / std::to_string(fd));
}

void MockProc::setPermissions(const fs::path& relativePath, fs::perms prms, fs::perm_options opts) {
    std::error_code ec;
    fs::permissions(root / relativePath, prms, opts, ec);
    if (ec) {
        // In a real test utils lib, you might throw or log here.
        // For this context, we'll ignore errors to keep it simple,
        // as permission errors on the test runner's temp dir are unlikely
        // and would cause other failures anyway.
        std::cerr << "Failed to set permissions on " << (root / relativePath) << ": " << ec.message() << std::endl;
    }
}

void MockProc::createExeSymlink(int pid, const fs::path& targetPath) {
    createSymlink(pid, "exe", targetPath.string());
}

void MockProc::createCwdSymlink(int pid, const fs::path& targetPath) {
    createSymlink(pid, "cwd", targetPath.string());
}

void MockProc::createRootSymlink(int pid, const fs::path& targetPath) {
    createSymlink(pid, "root", targetPath.string());
}

void MockProc::createComm(int pid, const std::string& commName) {
    createProcFile(pid, "comm", commName + "\n");
}

std::string MockProc::ProcMapEntry::toString() const {
    std::stringstream ss;
    ss << addressRange << " "
       << perms << " "
       << std::hex << std::setfill('0') << std::setw(addressPartWidth) << offset << " "
       << dev << " "
       << std::dec << inode;
    if (!pathname.empty()) {
        ss << " " << pathname.string();
    }
    return ss.str();
}

std::string MockProc::ProcIoStats::toString() const {
    std::stringstream ss;
    ss << "rchar: " << rchar << '\n';
    ss << "wchar: " << wchar << '\n';
    ss << "syscr: " << syscr << '\n';
    ss << "syscw: " << syscw << '\n';
    ss << "read_bytes: " << readBytes << '\n';
    ss << "write_bytes: " << writeBytes << '\n';
    ss << "cancelled_write_bytes: " << cancelledWriteBytes << '\n';
    return ss.str();
}

std::string MockProc::ProcStatData::toString() const {
    std::stringstream ss;
    ss << pid << " (" << comm << ") "
       << state << " "
       << ppid << " " << pgrp << " " << session << " " << tty_nr << " " << tpgid << " "
       << std::hex << std::setfill('0') << std::setw(flagsWidth) << flags << " "
       << std::dec << std::setfill('0')
       << minflt << " " << cminflt << " " << majflt << " " << cmajflt << " "
       << utime << " " << stime << " "
       << cutime << " " << cstime << " "
       << priority << " " << nice << " "
       << num_threads << " "
       << itrealvalue << " "
       << starttime << " " << vsize << " " << rss << " " << rsslim << " "
       << startcode << " " << endcode << " " << startstack << " "
       << kstkesp << " " << kstkeip << " "
       << signal << " " << blocked << " " << sigignore << " " << sigcatch << " "
       << wchan << " "
       << nswap << " " << cnswap << " "
       << exit_signal << " " << processor << " "
       << rt_priority << " " << policy << " "
       << delayacct_blkio_ticks << " "
       << guest_time << " " << cguest_time << " "
       << start_data << " " << end_data << " " << start_brk << " "
       << arg_start << " " << arg_end << " " << env_start << " " << env_end << " "
       << std::dec << exit_code;
    return ss.str();
}

std::string MockProc::MeminfoData::toString() const {
    std::stringstream ss;
    ss << "MemTotal:       " << memTotalKb << " kB\n";
    ss << "MemFree:        " << memFreeKb << " kB\n";
    ss << "MemAvailable:   " << memAvailableKb << " kB\n";
    ss << "Buffers:        " << buffersKb << " kB\n";
    ss << "Cached:         " << cachedKb << " kB\n";
    ss << "SwapTotal:      " << swapTotalKb << " kB\n";
    ss << "SwapFree:       " << swapFreeKb << " kB\n";
    return ss.str();
}

std::string MockProc::CpuinfoData::toString() const {
    std::stringstream ss;
    ss << "processor\t: " << processorId << '\n';
    ss << "vendor_id\t: " << vendorId << '\n';
    ss << "model name\t: " << modelName << '\n';
    ss << "cpu MHz\t\t: " << std::fixed << std::setprecision(3) << cpuMhz << '\n';
    ss << "siblings\t: " << siblings << '\n';
    ss << "cpu cores\t: " << cpuCores << '\n';
    ss << '\n';
    return ss.str();
}

std::string MockProc::SystemStatData::toString() const {
    std::stringstream ss;
    ss << user << " " << nice << " " << system << " " << idle << " "
       << iowait << " " << irq << " " << softirq << " " << steal << " "
       << guest << " " << guest_nice;
    return ss.str();
}

std::string MockProc::NetDevStats::toString() const {
    std::stringstream ss;
    ss << interface << ": "
       << rx_bytes << " " << rx_packets << " " << rx_errs << " " << rx_drop << " 0 0 0 0 "
       << tx_bytes << " " << tx_packets << " " << tx_errs << " " << tx_drop << " 0 0 0 0";
    return ss.str();
}

void MockProc::createStat(int pid, const ProcStatData& data) {
    createProcFile(pid, "stat", data.toString());
}

void MockProc::createIo(int pid, const ProcIoStats& stats) {
    createProcFile(pid, "io", stats.toString());
}

void MockProc::createMaps(int pid, const std::vector<ProcMapEntry>& entries) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    std::ofstream mapsFile(pidPath / "maps");
    for (const auto& entry : entries) {
        mapsFile << entry.toString() << '\n';
    }
}

void MockProc::createMeminfo(const MeminfoData& data) {
    createFile("meminfo", data.toString());
}

void MockProc::createCpuinfo(const std::vector<CpuinfoData>& cores) {
    std::ofstream cpuinfoFile(root / "cpuinfo");
    for (const auto& core : cores) {
        cpuinfoFile << core.toString();
    }
}

void MockProc::createSystemStat(const SystemStatData& data) {
    std::stringstream ss;
    ss << "cpu  " << data.toString() << "\n";
    ss << "ctxt " << data.ctxt << "\n";
    ss << "btime " << data.btime << "\n";
    ss << "processes " << data.processes << "\n";
    createFile("stat", ss.str());
}

void MockProc::createSystemStat(const std::vector<SystemStatData>& perCpuData) {
    std::stringstream ss;
    if (!perCpuData.empty()) {
        ss << "cpu  " << perCpuData[0].toString() << "\n";
        for (size_t i = 1; i < perCpuData.size(); ++i) {
            ss << "cpu" << (i - 1) << " " << perCpuData[i].toString() << "\n";
        }
        ss << "ctxt " << perCpuData[0].ctxt << "\n";
        ss << "btime " << perCpuData[0].btime << "\n";
        ss << "processes " << perCpuData[0].processes << "\n";
    }
    createFile("stat", ss.str());
}

void MockProc::createUptime(double uptimeSeconds, double idleSeconds) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << uptimeSeconds << " "
       << std::fixed << std::setprecision(2) << idleSeconds << "\n";
    createFile("uptime", ss.str());
}

void MockProc::createVersion(const std::string& versionString) {
    createFile("version", versionString + "\n");
}

void MockProc::createNetDev(const std::vector<NetDevStats>& devices) {
    std::stringstream ss;
    ss << "Inter-|   Receive                                                |  Transmit\n";
    ss << " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n";
    for (const auto& dev : devices) {
        ss << dev.toString() << "\n";
    }
    // Check if "net" directory exists, if not create it
    fs::path netPath = root / "net";
    if (!fs::exists(netPath)) {
        fs::create_directory(netPath);
    }
    createFile("net/dev", ss.str());
}

void MockProc::addThread(int parentPid, int threadId, const AddThreadOptions& options) {
    fs::path taskPath = root / std::to_string(parentPid) / "task" / std::to_string(threadId);
    fs::create_directories(taskPath);

    fs::path threadPath = root / std::to_string(threadId);
    fs::create_directories(threadPath);

    ProcStatData stat = options.statData;
    if (options.populateDefaultStat) {
        if (stat.pid == 0) stat.pid = threadId;
        if (stat.comm.empty() && !options.name.empty()) stat.comm = options.name;
        if (stat.ppid == 0) stat.ppid = parentPid;
        if (stat.state == '\0') stat.state = 'R';
    }
    
    std::ofstream(taskPath / "stat") << stat.toString();
    std::ofstream(threadPath / "stat") << stat.toString();

    auto createLinkToParent = [&](const std::string& name) {
        fs::path target = fs::path("..") / std::to_string(parentPid) / name;
        try {
            fs::create_symlink(target, threadPath / name);
            fs::create_symlink(target, taskPath / name); 
        } catch(...) { // NOLINT
            // ignore
        }
    };

    createLinkToParent("exe");
    createLinkToParent("cwd");
    createLinkToParent("root");
    createLinkToParent("maps");
    createLinkToParent("environ");

    if (!options.name.empty()) {
        std::ofstream(taskPath / "comm") << options.name << "\n";
        std::ofstream(threadPath / "comm") << options.name << "\n";
    }
}

void MockProc::addProcess(int pid, const AddProcessOptions& options) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directories(pidPath);

    std::map<std::string, std::string> statusData;
    statusData["Name"] = options.name;
    statusData["Pid"] = std::to_string(pid);
    if (options.ppid != -1) {
        statusData["PPid"] = std::to_string(options.ppid);
    } else if (options.statData.pid != 0 || options.populateDefaultStat) {
        statusData["PPid"] = std::to_string(options.statData.ppid);
    }
    if (options.statData.pid != 0 || options.populateDefaultStat) {
        statusData["State"] = std::string(1, options.statData.state);
        statusData["Tgid"] = std::to_string(pid);
    }
    for (const auto& pair : options.statusExtra) {
        statusData[pair.first] = pair.second;
    }
    createStatus(pid, statusData);

    if (!options.cmdlineArgs.empty()) {
        createCmdline(pid, options.cmdlineArgs);
    } else if (!options.name.empty()) {
        createCmdline(pid, {options.name});
    } else {
        createCmdline(pid, {});
    }

    if (!options.exePath.empty()) createExeSymlink(pid, options.exePath);
    if (!options.cwdPath.empty()) createCwdSymlink(pid, options.cwdPath);
    if (!options.rootPath.empty()) createRootSymlink(pid, options.rootPath);

    if (!options.environVars.empty()) createEnviron(pid, options.environVars);
    if (!options.fds.empty()) createFdDir(pid, options.fds);
    
    if (!options.name.empty()) createComm(pid, options.name);

    if (options.ioStats.rchar != 0 || options.ioStats.wchar != 0 || options.ioStats.syscr != 0 ||
        options.ioStats.syscw != 0 || options.ioStats.readBytes != 0 || options.ioStats.writeBytes != 0 ||
        options.ioStats.cancelledWriteBytes != 0) {
        createIo(pid, options.ioStats);
    }

    bool shouldCreateStat = (options.statData.pid != 0) || options.populateDefaultStat;
    if (shouldCreateStat) {
        ProcStatData statToCreate = options.statData;
        if (options.populateDefaultStat) {
            if (statToCreate.pid == 0) statToCreate.pid = pid;
            if (statToCreate.comm.empty() && !options.name.empty()) statToCreate.comm = options.name;
            if (statToCreate.ppid == 0) statToCreate.ppid = 1; 
            if (statToCreate.state == '\0') statToCreate.state = 'R';
        }
        if (options.ppid != -1) {
            statToCreate.ppid = options.ppid;
        }
        createStat(pid, statToCreate);
    }

    if (!options.maps.empty()) {
        createMaps(pid, options.maps);
    }
}

ProcessBuilder MockProc::buildProcess(int pid) {
    return {*this, pid};
}

ProcessBuilder::ProcessBuilder(MockProc& mockProc, int pid) : mockProc_(mockProc), pid_(pid) {
    options_.populateDefaultStat = true;
}

ProcessBuilder& ProcessBuilder::withName(const std::string& name) {
    options_.name = name;
    return *this;
}
ProcessBuilder& ProcessBuilder::withComm(const std::string& commName) {
    options_.name = commName; 
    return *this;
}
ProcessBuilder& ProcessBuilder::withCmdline(const std::vector<std::string>& args) {
    options_.cmdlineArgs = args;
    return *this;
}
ProcessBuilder& ProcessBuilder::withParent(int ppid) {
    options_.ppid = ppid;
    return *this;
}
ProcessBuilder& ProcessBuilder::withExe(const fs::path& exePath) {
    options_.exePath = exePath;
    return *this;
}
ProcessBuilder& ProcessBuilder::withCwd(const fs::path& cwdPath) {
    options_.cwdPath = cwdPath;
    return *this;
}
ProcessBuilder& ProcessBuilder::withRoot(const fs::path& rootPath) {
    options_.rootPath = rootPath;
    return *this;
}
ProcessBuilder& ProcessBuilder::withEnviron(const std::map<std::string, std::string>& envVars) {
    options_.environVars = envVars;
    return *this;
}
ProcessBuilder& ProcessBuilder::withFd(int fd, const std::string& target) {
    options_.fds.emplace_back(fd, target);
    return *this;
}
ProcessBuilder& ProcessBuilder::withIoStats(const MockProc::ProcIoStats& stats) {
    options_.ioStats = stats;
    return *this;
}
ProcessBuilder& ProcessBuilder::withStat(const MockProc::ProcStatData& data) {
    options_.statData = data;
    return *this;
}
ProcessBuilder& ProcessBuilder::withStatusField(const std::string& key, const std::string& value) {
    options_.statusExtra[key] = value;
    return *this;
}
ProcessBuilder& ProcessBuilder::withMap(const MockProc::ProcMapEntry& mapEntry) {
    options_.maps.push_back(mapEntry);
    return *this;
}
ProcessBuilder& ProcessBuilder::withMaps(const std::vector<MockProc::ProcMapEntry>& mapEntries) {
    options_.maps = mapEntries;
    return *this;
}
void ProcessBuilder::create() {
    mockProc_.addProcess(pid_, options_);
}
