// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/analyzer.h"
#include "analyzer/process_model.h"
#include "utils/types.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/system.h"
#include "utils/time.h"
#include "internal_helpers.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <unistd.h>     // For readlink, getuid
#include <sys/stat.h>   // For stat
#include <pwd.h>        // For getpwuid
#include <algorithm>    // For std::remove
#include <chrono>       // For std::chrono
#include <array>        // For std::array

namespace {

// Helper to read symlink target
utils::Result<std::string> readSymlink(const std::filesystem::path& linkPath) {
    std::array<char, 256> buffer; // Increased buffer size
    ssize_t len = ::readlink(linkPath.c_str(), buffer.data(), buffer.size() - 1);
    if (len == -1) {
        return std::unexpected(std::error_code(errno, std::system_category()));
    }
    buffer[len] = '\0';
    return std::string(buffer.data());
}

// Helper to get username from UID
std::string getUserName(uid_t uid) {
    if (struct passwd *pw = getpwuid(uid)) {
        return pw->pw_name;
    }
    return std::to_string(uid); // Fallback to UID string if name not found
}

// Helper to parse /proc/[pid]/stat
// See man proc(5) for format details
utils::Result<ProcessInfo> parseStatFile(pid_t pid, const std::string& statContent) {
    ProcessInfo info;
    info.pid = pid;

    std::istringstream iss(statContent);
    std::string token;
    
    // Read pid
    iss >> info.pid;
    if (iss.fail() || info.pid != pid) { // Basic validation
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    // Read comm (process name), which can contain spaces and is enclosed in parentheses
    size_t start_paren = statContent.find('(');
    size_t end_paren = statContent.rfind(')');
    if (start_paren == std::string::npos || end_paren == std::string::npos || end_paren < start_paren) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    info.name = statContent.substr(start_paren + 1, end_paren - start_paren - 1);

    // Skip to after the closing parenthesis
    iss.seekg(end_paren + 1);

    // Read remaining fields
    char stateChar;
    long ppid_l, pgrp_l, session_l, tty_nr_l, tpgid_l;
    unsigned long flags_ul;
    unsigned long minflt_ul, cminflt_ul, majflt_ul, cmajflt_ul;
    unsigned long utime_ul, stime_ul, cutime_ul, cstime_ul;
    long priority_l, nice_l;
    long num_threads_l;
    long itrealvalue_l;
    unsigned long long starttime_ull; // in clock ticks
    unsigned long vsize_ul;
    long rss_l; // in pages

    if (!(iss >> stateChar >> ppid_l >> pgrp_l >> session_l >> tty_nr_l >> tpgid_l
              >> flags_ul >> minflt_ul >> cminflt_ul >> majflt_ul >> cmajflt_ul
              >> utime_ul >> stime_ul >> cutime_ul >> cstime_ul
              >> priority_l >> nice_l >> num_threads_l >> itrealvalue_l
              >> starttime_ull >> vsize_ul >> rss_l)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    info.state = std::string(1, stateChar);
    info.ppid = static_cast<pid_t>(ppid_l);
    info.cpuUserTimeTicks = utime_ul;
    info.cpuKernelTimeTicks = stime_ul;
    info.priority = priority_l; // Not always the same as nice, but usually derived from it.
    // nice_l is the actual nice value. Let's use priority_l for ProcessInfo for now.
    info.threadCount = num_threads_l;
    info.startTimeTicks = starttime_ull;
    info.virtualMemory = vsize_ul / 1024; // Convert bytes to KB
    info.residentMemory = rss_l * (sysconf(_SC_PAGESIZE) / 1024); // Convert pages to KB

    // startTimeUnix and elapsedTime will be calculated later in getProcessDetails,
    // as they need system boot time.

    return info;
}


// Helper to parse /proc/[pid]/status
// Provides uid and some memory info more clearly
utils::Result<ProcessInfo> parseStatusFile(ProcessInfo& info, const std::string& statusContent) {
    std::istringstream iss(statusContent);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("Uid:", 0) == 0) {
            std::istringstream lineStream(line);
            std::string label;
            uid_t ruid, euid, suid, fsuid;
            lineStream >> label >> ruid >> euid >> suid >> fsuid;
            if (!lineStream.fail()) {
                info.uid = ruid;
                info.username = getUserName(ruid);
            }
        } else if (line.rfind("VmSize:", 0) == 0) {
            std::istringstream lineStream(line);
            std::string label, unit;
            long long value;
            lineStream >> label >> value >> unit;
            if (!lineStream.fail() && unit == "kB") {
                info.virtualMemory = value;
            }
        } else if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream lineStream(line);
            std::string label, unit;
            long long value;
            lineStream >> label >> value >> unit;
            if (!lineStream.fail() && unit == "kB") {
                info.residentMemory = value;
            }
        } else if (line.rfind("Threads:", 0) == 0) {
            std::istringstream lineStream(line);
            std::string label;
            long value;
            lineStream >> label >> value;
            if (!lineStream.fail()) {
                info.threadCount = value;
            }
        }
        // Add more parsing for other fields as needed, e.g., Cpus_allowed_list, Mems_allowed_list, etc.
    }
    return info;
}


// Helper to parse /proc/[pid]/io
utils::Result<void> parseIoFile(ProcessInfo& info, const std::string& ioContent) {
    std::istringstream iss(ioContent);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("read_bytes:", 0) == 0) {
            std::istringstream lineStream(line);
            std::string label;
            long long value;
            lineStream >> label >> value;
            if (!lineStream.fail()) info.ioReadBytes = value;
        } else if (line.rfind("write_bytes:", 0) == 0) {
            std::istringstream lineStream(line);
            std::string label;
            long long value;
            lineStream >> label >> value;
            if (!lineStream.fail()) info.ioWriteBytes = value;
        }
    }
    return {};
}

// Helper to parse /proc/[pid]/cmdline
utils::Result<std::string> parseCmdlineFile(const std::string& cmdlineContent) {
    // cmdline content is null-separated arguments
    std::string cmdline = cmdlineContent;
    std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
    if (!cmdline.empty() && cmdline.back() == ' ') {
        cmdline.pop_back(); // Remove trailing space
    }
    return cmdline;
}

} // anonymous namespace

// Implementation of ProcessAnalyzer::getProcessDetails
utils::Result<ProcessInfo> ProcessAnalyzer::getProcessDetails(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    std::filesystem::path pidPath = procPath / std::to_string(pid);
    ProcessInfo info;
    info.pid = pid;

    // Read /proc/[pid]/stat
    auto statContent = utils::readTextFile((pidPath / "stat").string());
    if (!statContent) return std::unexpected(statContent.error());
    
    auto statResult = parseStatFile(pid, *statContent);
    if (!statResult) return std::unexpected(statResult.error());
    info = statResult.value(); // Assign parsed info

    // Read /proc/[pid]/status for Uid, more accurate memory and threads
    auto statusContent = utils::readTextFile((pidPath / "status").string());
    if (statusContent) { // status file might not always be readable, so proceed if available
        auto statusResult = parseStatusFile(info, *statusContent);
        if (!statusResult) {
            // Log parsing error but try to continue with stat info
            // For now, we will just return the error.
            return std::unexpected(statusResult.error());
        }
        info = statusResult.value();
    }

    // Read /proc/[pid]/cmdline
    auto cmdlineContent = utils::readTextFile((pidPath / "cmdline").string());
    if (cmdlineContent) {
        auto cmdlineResult = parseCmdlineFile(*cmdlineContent);
        if (!cmdlineResult) return std::unexpected(cmdlineResult.error());
        info.cmdline = cmdlineResult.value();
    }

    // Read /proc/[pid]/exe (symlink)
    auto exePathResult = readSymlink(pidPath / "exe");
    if (exePathResult) {
        info.executablePath = exePathResult.value();
    } else {
        // If executable path can't be read, it's not critical.
        // It's possible for some processes (e.g., kernel threads, or after execve)
        info.executablePath = "";
    }

    // Read /proc/[pid]/cwd (symlink)
    auto cwdPathResult = readSymlink(pidPath / "cwd");
    if (cwdPathResult) {
        info.currentWorkingDirectory = cwdPathResult.value();
    } else {
        info.currentWorkingDirectory = "";
    }

    // Read /proc/[pid]/io
    auto ioContent = utils::readTextFile((pidPath / "io").string());
    if (ioContent) {
        auto ioResult = parseIoFile(info, *ioContent);
        if (!ioResult) return std::unexpected(ioResult.error());
    }

    // Calculate start time in Unix epoch
    auto bootTimeResult = getSystemBootTimeUnix();
    if (bootTimeResult) {
        long long systemBootTime = bootTimeResult.value();
        auto clockTicksResult = getSystemClockTicksPerSecond();
        if (clockTicksResult) {
            long systemClockTicksPerSecond = clockTicksResult.value();
            if (systemClockTicksPerSecond > 0) {
                // startTimeTicks is in USER_HZ (clock ticks) since system boot.
                // Convert to seconds since boot, then add system boot time.
                info.startTimeUnix = systemBootTime + (info.startTimeTicks / systemClockTicksPerSecond);
                
                // Calculate elapsed time (uptime)
                auto currentTime = std::chrono::system_clock::now();
                auto durationSinceEpoch = currentTime.time_since_epoch();
                long long currentUnixTime = std::chrono::duration_cast<std::chrono::seconds>(durationSinceEpoch).count();

                if (info.startTimeUnix > 0 && currentUnixTime >= info.startTimeUnix) {
                    long long uptimeSeconds = currentUnixTime - info.startTimeUnix;
                    info.elapsedTime = utils::formatElapsedTime(uptimeSeconds).value_or("N/A");
                }
            }
        }
    }
    // CPU usage and Memory percentage are usually calculated externally or require system-wide totals
    // For now, leave as default 0.0F.

    return info;
}

// Implementation of ProcessAnalyzer::getProcessThreads
utils::Result<std::vector<ThreadInfo>> ProcessAnalyzer::getProcessThreads(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    std::vector<ThreadInfo> threads;
    std::filesystem::path taskPath = procPath / std::to_string(pid) / "task";

    if (!std::filesystem::exists(taskPath) || !std::filesystem::is_directory(taskPath)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    for (const auto& entry : std::filesystem::directory_iterator(taskPath)) {
        if (entry.is_directory()) {
            std::string tidStr = entry.path().filename().string();
            auto tidOpt = utils::parseIntegerNoThrow<pid_t>(tidStr, 10);
            if (!tidOpt) continue; // Not a valid TID directory

            pid_t tid = *tidOpt;
            ThreadInfo threadInfo;
            threadInfo.tid = tid;

            // Read /proc/[pid]/task/[tid]/stat
            auto statContent = utils::readTextFile((entry.path() / "stat").string());
            if (statContent) {
                std::istringstream iss(*statContent);
                std::string token;
                
                // Skip pid
                iss >> token;
                
                // Read comm (thread name)
                size_t start_paren = statContent->find('(');
                size_t end_paren = statContent->rfind(')');
                if (start_paren != std::string::npos && end_paren != std::string::npos && end_paren > start_paren) {
                    threadInfo.name = statContent->substr(start_paren + 1, end_paren - start_paren - 1);
                } else {
                    threadInfo.name = "Unknown";
                }

                iss.seekg(end_paren + 1);

                char stateChar;
                unsigned long utime_ul, stime_ul;
                if (iss >> stateChar) {
                    threadInfo.state = std::string(1, stateChar);
                }
                // Skip many fields to get utime and stime
                // (p_pid to policy - 14 fields)
                for (int i = 0; i < 14; ++i) iss >> token;
                if (iss >> utime_ul >> stime_ul) {
                    threadInfo.cpuUserTimeTicks = utime_ul;
                    threadInfo.cpuKernelTimeTicks = stime_ul;
                }
            }
            threads.push_back(threadInfo);
        }
    }
    return threads;
}


// Implementation of ProcessAnalyzer::getProcessOpenFileDetails
utils::Result<std::vector<OpenFileDescriptorInfo>> ProcessAnalyzer::getProcessOpenFileDetails(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    std::vector<OpenFileDescriptorInfo> openFiles;
    std::filesystem::path fdPath = procPath / std::to_string(pid) / "fd";

    if (!std::filesystem::exists(fdPath) || !std::filesystem::is_directory(fdPath)) {
        // If fd directory doesn't exist or not accessible, return empty list or permission denied
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }

    for (const auto& entry : std::filesystem::directory_iterator(fdPath)) {
        if (entry.is_symlink()) {
            std::string fdStr = entry.path().filename().string();
            auto fdOpt = utils::parseIntegerNoThrow<int>(fdStr, 10);
            if (!fdOpt) continue; // Not a valid file descriptor

            OpenFileDescriptorInfo fdInfo;
            fdInfo.fd = *fdOpt;

            auto linkTargetResult = readSymlink(entry.path());
            if (linkTargetResult) {
                fdInfo.path = linkTargetResult.value();

                // Determine file type (basic classification)
                if (fdInfo.path.rfind("socket:[", 0) == 0) {
                    fdInfo.type = OpenFileType::Socket;
                } else if (fdInfo.path.rfind("pipe:[", 0) == 0) {
                    fdInfo.type = OpenFileType::Pipe;
                } else if (fdInfo.path.rfind("anon_inode:[", 0) == 0) {
                    fdInfo.type = OpenFileType::AnonInode;
                } else if (fdInfo.path.rfind("/", 0) == 0) {
                    // Try to differentiate between regular files and device files
                    try {
                        std::error_code ec;
                        auto status = std::filesystem::status(fdInfo.path, ec);
                        if ((!ec && std::filesystem::is_block_file(status)) || std::filesystem::is_character_file(status)) {
                            fdInfo.type = OpenFileType::Device;
                        } else if (!ec && std::filesystem::is_regular_file(status)) {
                            fdInfo.type = OpenFileType::File;
                        } else {
                            fdInfo.type = OpenFileType::Other; // Catch-all for unknown path types
                        }
                    } catch (...) {
                         fdInfo.type = OpenFileType::Other; // Fallback
                    }
                } else {
                    fdInfo.type = OpenFileType::Other;
                }
            } else {
                // If symlink target cannot be read, path is unknown.
                fdInfo.path = "[unknown]";
                fdInfo.type = OpenFileType::Unknown;
            }
            openFiles.push_back(fdInfo);
        }
    }
    return openFiles;
}