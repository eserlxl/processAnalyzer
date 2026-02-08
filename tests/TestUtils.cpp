// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "TestUtils.h"

#include <sstream>
#include <iomanip>
#include <algorithm> // For std::for_each
#include <string> // Include string for std::string
#include <vector> // Include vector for std::vector
#include <map> // Include map for std::map
#include <filesystem> // Include filesystem for fs::path

namespace fs = std::filesystem;

constexpr int kAddressWidth = 16;
constexpr int kFlagsWidth = 8;
constexpr int kAddressPartWidth = 8;

// --- MockProc Method Implementations ---

// New PID-Specific Symlink Creators
void MockProc::createExeSymlink(int pid, const fs::path& targetPath) {
    createSymlink(pid, "exe", targetPath.string());
}

void MockProc::createCwdSymlink(int pid, const fs::path& targetPath) {
    createSymlink(pid, "cwd", targetPath.string());
}

void MockProc::createRootSymlink(int pid, const fs::path& targetPath) {
    createSymlink(pid, "root", targetPath.string());
}

// New Creator for /proc/<pid>/comm
void MockProc::createComm(int pid, const std::string& commName) {
    createProcFile(pid, "comm", commName + "\n");
}

// Structured Content Generation for Complex Files

// ProcMapEntry::toString()
std::string MockProc::ProcMapEntry::toString() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    // Address range: 7f000000-7f010000 (8-byte hex for each part)
    // We expect a specific format for address range, let's assume it's already formatted or needs minimal padding.
    // For simplicity, assume addressRange is already in correct format like "7f000000-7f010000".
    // If not, we would need to parse and format hex values here.
    // The parts of the address range are 8 hex characters each.
    ss << std::setw(kAddressPartWidth) << addressRange.substr(0, addressRange.find('-')) << "-"
       << std::setw(kAddressPartWidth) << addressRange.substr(addressRange.find('-') + 1);
    ss << " ";
    ss << perms;
    ss << " ";
    // Offset: 00001000 (8-byte hex)
    ss << std::setw(kAddressPartWidth) << offset;
    ss << " ";
    ss << dev;
    ss << " ";
    // Inode: 0 (decimal)
    ss << std::dec << inode;
    if (!pathname.empty()) {
        ss << " " << pathname.string();
    }
    return ss.str();
}

void MockProc::createMaps(int pid, const std::vector<ProcMapEntry>& entries) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directory(pidPath);
    std::ofstream mapsFile(pidPath / "maps");
    for (const auto& entry : entries) {
        mapsFile << entry.toString() << '\n';
    }
}

// ProcIoStats::toString()
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

void MockProc::createIo(int pid, const ProcIoStats& stats) {
    createProcFile(pid, "io", stats.toString());
}

// ProcStatData::toString()
std::string MockProc::ProcStatData::toString() const {
    std::stringstream ss;
    ss << pid << " (" << comm << ") "
       << state << " "
       << ppid << " " << pgrp << " " << session << " " << tty_nr << " " << tpgid << " "
       << std::hex << std::setfill('0') << std::setw(kFlagsWidth) << flags << " " // flags as 8-digit hex
       << std::dec << std::setfill('0') // Reset to decimal
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
       << std::dec // Reset to decimal
       << exit_code; // Last field, no trailing space
    return ss.str();
}

// Explicitly qualify ProcStatData when defining its member function
void MockProc::createStat(int pid, const ProcStatData& data) {
    createProcFile(pid, "stat", data.toString());
}

// System-wide /proc File Creators

// MeminfoData::toString()
std::string MockProc::MeminfoData::toString() const {
    std::stringstream ss;
    ss << "MemTotal:       " << memTotalKb << " kB\n";
    ss << "MemFree:        " << memFreeKb << " kB\n";
    ss << "MemAvailable:   " << memAvailableKb << " kB\n";
    ss << "Buffers:        " << buffersKb << " kB\n";
    ss << "Cached:         " << cachedKb << " kB\n";
    ss << "SwapTotal:      " << swapTotalKb << " kB\n";
    ss << "SwapFree:       " << swapFreeKb << " kB\n";
    // Add other common fields if they are populated in MeminfoData
    return ss.str();
}

void MockProc::createMeminfo(const MeminfoData& data) {
    createFile("meminfo", data.toString());
}

// CpuinfoData::toString()
std::string MockProc::CpuinfoData::toString() const {
    std::stringstream ss;
    ss << "processor\t: " << processorId << '\n';
    ss << "vendor_id\t: " << vendorId << '\n';
    ss << "model name\t: " << modelName << '\n';
    ss << "cpu MHz\t\t: " << std::fixed << std::setprecision(3) << cpuMhz << '\n';
    ss << "siblings\t: " << siblings << '\n';
    ss << "cpu cores\t: " << cpuCores << '\n';
    // Add other common fields
    ss << '\n'; // Separator between CPU entries
    return ss.str();
}

void MockProc::createCpuinfo(const std::vector<CpuinfoData>& cores) {
    fs::path cpuinfoPath = root / "cpuinfo";
    std::ofstream cpuinfoFile(cpuinfoPath);
    for (const auto& core : cores) {
        cpuinfoFile << core.toString();
    }
}

// Enhanced addProcess Functionality
void MockProc::addProcess(int pid, const AddProcessOptions& options) {
    fs::path pidPath = root / std::to_string(pid);
    fs::create_directories(pidPath);

    // 1. Create /proc/<pid>/status
    std::map<std::string, std::string> statusData;
    statusData["Name"] = options.name;
    statusData["Pid"] = std::to_string(pid);
    // Add other common status fields if necessary or from options.statusExtra
    // For example, Parent Pid (ppid) might be derived from options.statData.ppid if available.
    if (options.statData.pid != 0 || options.populateDefaultStat) {
        statusData["PPid"] = std::to_string(options.statData.ppid);
        statusData["State"] = std::string(1, options.statData.state);
        statusData["Tgid"] = std::to_string(options.statData.pid); // Assuming Tgid is same as Pid for single process
    }
    for (const auto& pair : options.statusExtra) {
        statusData[pair.first] = pair.second;
    }
    createStatus(pid, statusData);

    // 2. Create /proc/<pid>/cmdline
    if (!options.cmdlineArgs.empty()) {
        createCmdline(pid, options.cmdlineArgs);
    } else if (!options.name.empty()) {
        // Fallback if cmdlineArgs is empty but name is provided
        createCmdline(pid, {options.name});
    } else {
        // Create an empty cmdline if neither is provided
        createCmdline(pid, {});
    }

    // 3. Create symlinks: exe, cwd, root
    if (!options.exePath.empty()) {
        createExeSymlink(pid, options.exePath);
    }
    if (!options.cwdPath.empty()) {
        createCwdSymlink(pid, options.cwdPath);
    }
    if (!options.rootPath.empty()) {
        createRootSymlink(pid, options.rootPath);
    }

    // 4. Create /proc/<pid>/environ
    if (!options.environVars.empty()) {
        createEnviron(pid, options.environVars);
    }

    // 5. Create /proc/<pid>/fd directory
    if (!options.fds.empty()) {
        createFdDir(pid, options.fds);
    }

    // 6. Create /proc/<pid>/comm
    if (!options.name.empty()) {
        createComm(pid, options.name);
    } else {
        // If name is empty, use a default or derived comm name. For now, assume name is always provided or not critical.
        // If it's critical, this might need adjustment or a default value.
    }

    // 7. Create /proc/<pid>/io
    // Check if ioStats has any non-default values to avoid creating an empty file unnecessarily
    if (options.ioStats.rchar != 0 || options.ioStats.wchar != 0 || options.ioStats.syscr != 0 ||
        options.ioStats.syscw != 0 || options.ioStats.readBytes != 0 || options.ioStats.writeBytes != 0 ||
        options.ioStats.cancelledWriteBytes != 0) {
        createIo(pid, options.ioStats);
    }

    // 8. Create /proc/<pid>/stat
    // Create stat file if statData is explicitly set or if populateDefaultStat is true
    bool shouldCreateStat = (options.statData.pid != 0) || options.populateDefaultStat;
    
    if (shouldCreateStat) {
        ProcStatData statToCreate = options.statData;
        // If populateDefaultStat is true and statData is not fully specified, populate defaults.
        if (options.populateDefaultStat) {
            if (statToCreate.pid == 0) statToCreate.pid = pid;
            if (statToCreate.comm.empty() && !options.name.empty()) statToCreate.comm = options.name;
            if (statToCreate.ppid == 0 && options.statData.pid == 0) { // Only set ppid if not explicitly given and statData wasn't set.
                 // A default parent PID could be 1 (init/systemd) or derived if multiple processes are mocked.
                 // For simplicity, let's use a common default if not otherwise specified.
                 // This might need to be more robust in complex test scenarios.
                 statToCreate.ppid = 1; 
            }
            // Set default state to 'R' if not specified and populateDefaultStat is true
            if (statToCreate.state == '\0') { // Assuming '\0' or some default value means not set
                statToCreate.state = 'R';
            }
             // Other fields like flags, etc., could also have defaults if needed.
        }
        createStat(pid, statToCreate);
    }
}
