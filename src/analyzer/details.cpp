// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/types.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/time.h"
#include "analyzer/internal/helpers.h"

#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <unistd.h>     // For readlink, getuid
#include <pwd.h>        // For getpwuid
#include <algorithm>
#include <chrono>       // For std::chrono

namespace {

constexpr int kilobyteSize = 1024;
constexpr int parseIntegerBase = 10;
// Fields to skip after state (field 3) to reach utime (field 14):
// ppid, pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt
constexpr int threadStatSkipFields = 10;

// Helper to get username from UID
std::string getUserName(uid_t uid) {
    if (struct passwd *pw = getpwuid(uid)) {
        return pw->pw_name;
    }
    return std::to_string(uid); // Fallback to UID string if name not found
}

// Helper to parse /proc/[pid]/stat
// See man proc(5) for format details
utils::Result<ProcessInfo> parseStatFile(pid_t pid, std::string_view statContent) {
    ProcessInfo info;
    info.pid = pid;

    std::istringstream iss{std::string(statContent)};
    
    // Read pid
    iss >> info.pid;
    if (iss.fail() || info.pid != pid) { // Basic validation
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    // Read comm (process name), which can contain spaces and is enclosed in parentheses
    size_t startParen = statContent.find('(');
    size_t endParen = statContent.rfind(')');
    if (startParen == std::string::npos || endParen == std::string::npos || endParen < startParen) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    info.name = statContent.substr(startParen + 1, endParen - startParen - 1);

    // Skip to after the closing parenthesis
    iss.seekg(static_cast<std::streamoff>(endParen + 1));

    // Read remaining fields
    char stateChar;
    long ppidL;
    long pgrpL;
    long sessionL;
    long ttyNrL;
    long tpgidL;
    unsigned long flagsUl;
    unsigned long minfltUl;
    unsigned long cminfltUl;
    unsigned long majfltUl;
    unsigned long cmajfltUl;
    unsigned long utimeUl;
    unsigned long stimeUl;
    unsigned long cutimeUl;
    unsigned long cstimeUl;
    long priorityL;
    long niceL;
    long numThreadsL;
    long itrealvalueL;
    unsigned long long starttimeUll; // in clock ticks
    unsigned long vsizeUl;
    long rssL; // in pages

    if (!(iss >> stateChar >> ppidL >> pgrpL >> sessionL >> ttyNrL >> tpgidL
              >> flagsUl >> minfltUl >> cminfltUl >> majfltUl >> cmajfltUl
              >> utimeUl >> stimeUl >> cutimeUl >> cstimeUl
              >> priorityL >> niceL >> numThreadsL >> itrealvalueL
              >> starttimeUll >> vsizeUl >> rssL)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }

    info.state = std::string(1, stateChar);
    info.ppid = static_cast<pid_t>(ppidL);
    info.cpuUserTimeTicks = static_cast<long long>(utimeUl);
    info.cpuKernelTimeTicks = static_cast<long long>(stimeUl);
    info.priority = static_cast<int>(priorityL);
    info.threadCount = static_cast<int>(numThreadsL);
    info.startTimeTicks = static_cast<long long>(starttimeUll);
    info.virtualMemory = static_cast<long long>(vsizeUl) / kilobyteSize; // Convert bytes to KB
    info.residentMemory = rssL * (sysconf(_SC_PAGESIZE) / kilobyteSize); // Convert pages to KB

    // startTimeUnix and elapsedTime will be calculated later in getProcessDetails,
    // as they need system boot time.

    return info;
}


// Helper to parse /proc/[pid]/status
// Provides uid and some memory info more clearly
utils::Result<void> parseStatusFile(ProcessInfo& info, std::string_view statusContent) {
    std::istringstream iss{std::string(statusContent)};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("Uid:")) {
            std::istringstream lineStream(line);
            std::string label;
            uid_t ruid;
            uid_t euid;
            uid_t suid;
            uid_t fsuid;
            lineStream >> label >> ruid >> euid >> suid >> fsuid;
            if (!lineStream.fail()) {
                info.uid = ruid;
                info.username = getUserName(ruid);
            }
        } else if (line.starts_with("VmSize:")) {
            std::istringstream lineStream(line);
            std::string label;
            std::string unit;
            long long value;
            lineStream >> label >> value >> unit;
            if (!lineStream.fail() && unit == "kB") {
                info.virtualMemory = value;
            }
        } else if (line.starts_with("VmRSS:")) {
            std::istringstream lineStream(line);
            std::string label;
            std::string unit;
            long long value;
            lineStream >> label >> value >> unit;
            if (!lineStream.fail() && unit == "kB") {
                info.residentMemory = value;
            }
        } else if (line.starts_with("Threads:")) {
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
    return {};
}



// Helper to parse /proc/[pid]/io
utils::Result<void> parseIoFile(ProcessInfo& info, std::string_view ioContent) {
    std::istringstream iss{std::string(ioContent)};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("read_bytes:")) {
            std::istringstream lineStream(line);
            std::string label;
            long long value;
            lineStream >> label >> value;
            if (!lineStream.fail()) info.ioReadBytes = value;
        } else if (line.starts_with("write_bytes:")) {
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
utils::Result<std::string> parseCmdlineFile(std::string_view cmdlineContent) {
    // cmdline content is null-separated arguments
    std::string cmdline{cmdlineContent};
    std::ranges::replace(cmdline, '\0', ' ');
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
    }

    // Read /proc/[pid]/cmdline
    auto cmdlineContent = utils::readTextFile((pidPath / "cmdline").string());
    if (cmdlineContent) {
        auto cmdlineResult = parseCmdlineFile(*cmdlineContent);
        if (!cmdlineResult) return std::unexpected(cmdlineResult.error());
        info.cmdline = cmdlineResult.value();
    }

    // Read /proc/[pid]/exe (symlink)
    std::error_code ec;
    auto exePath = std::filesystem::read_symlink(pidPath / "exe", ec);
    if (!ec) {
        info.executablePath = exePath.string();
    } else {
        // If executable path can't be read, it's not critical.
        // It's possible for some processes (e.g., kernel threads, or after execve)
        info.executablePath = "";
    }

    // Read /proc/[pid]/cwd (symlink)
    auto cwdPath = std::filesystem::read_symlink(pidPath / "cwd", ec);
    if (!ec) {
        info.currentWorkingDirectory = cwdPath.string();
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
            auto tidOpt = utils::parseInteger<pid_t>(tidStr, parseIntegerBase);
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
                size_t startParen = statContent->find('(');
                size_t endParen = statContent->rfind(')');
                bool commWellFormed = startParen != std::string::npos &&
                                      endParen != std::string::npos && endParen > startParen;
                if (commWellFormed) {
                    threadInfo.name = statContent->substr(startParen + 1, endParen - startParen - 1);
                } else {
                    threadInfo.name = "Unknown";
                }

                // Only parse the post-comm fields when the comm parens are well formed.
                // Otherwise endParen is std::string::npos and seekg(endParen + 1) wraps to
                // seekg(0), rewinding the stream so a pid digit is read as the thread state.
                if (commWellFormed) {
                    iss.seekg(static_cast<std::streamoff>(endParen + 1));

                    char stateChar;
                    unsigned long utimeUl;
                    unsigned long stimeUl;
                    if (iss >> stateChar) {
                        threadInfo.state = std::string(1, stateChar);
                    }
                    // Skip fields 4–13 (ppid through cmajflt) to reach utime (field 14).
                    for (int i = 0; i < threadStatSkipFields; ++i) iss >> token;
                    if (iss >> utimeUl >> stimeUl) {
                        threadInfo.cpuUserTimeTicks = static_cast<long long>(utimeUl);
                        threadInfo.cpuKernelTimeTicks = static_cast<long long>(stimeUl);
                    }
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

    std::error_code ec;
    if (!std::filesystem::exists(fdPath, ec) || !std::filesystem::is_directory(fdPath, ec)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }

    for (const auto& entry : std::filesystem::directory_iterator(fdPath, ec)) {
        std::string fdStr = entry.path().filename().string();
        auto fdOpt = utils::parseInteger<int>(fdStr, parseIntegerBase);
        if (!fdOpt) continue;

        OpenFileDescriptorInfo fdInfo;
        fdInfo.fd = *fdOpt;

        std::error_code readEc;
        auto linkTarget = std::filesystem::read_symlink(entry.path(), readEc);
        if (!readEc) {
            fdInfo.path = linkTarget.string();

            if (fdInfo.path.starts_with("socket:[")) {
                fdInfo.type = OpenFileType::socket;
            } else if (fdInfo.path.starts_with("pipe:[")) {
                fdInfo.type = OpenFileType::pipe;
            } else if (fdInfo.path.starts_with("anon_inode:[")) {
                fdInfo.type = OpenFileType::anonInode;
            } else if (fdInfo.path.starts_with('/')) {
                std::error_code statusEc;
                auto status = std::filesystem::status(fdInfo.path, statusEc);
                if (!statusEc) {
                    if (std::filesystem::is_block_file(status) || std::filesystem::is_character_file(status)) {
                        fdInfo.type = OpenFileType::device;
                    } else if (std::filesystem::is_regular_file(status)) {
                        fdInfo.type = OpenFileType::file;
                    } else {
                        fdInfo.type = OpenFileType::other;
                    }
                } else {
                    fdInfo.type = OpenFileType::other;
                }
            } else {
                fdInfo.type = OpenFileType::other;
            }
        } else {
            fdInfo.path = "[unknown]";
            fdInfo.type = OpenFileType::unknown;
        }
        openFiles.push_back(fdInfo);
    }
    return openFiles;
}

// Implementation of ProcessAnalyzer::getProcessEnvironment
utils::Result<std::vector<std::string>> ProcessAnalyzer::getProcessEnvironment(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    std::filesystem::path environPath = procPath / std::to_string(pid) / "environ";
    auto content = utils::readTextFile(environPath.string());
    if (!content) return std::unexpected(content.error());

    // /proc/[pid]/environ holds the environment as null-separated KEY=VALUE
    // entries; skip the empty trailing token after the final separator.
    return utils::split(*content, '\0', /*skipEmpty=*/true);
}

// Implementation of ProcessAnalyzer::getProcessResourceLimits
utils::Result<ResourceLimitInfo> ProcessAnalyzer::getProcessResourceLimits(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    auto content = utils::readTextFile((procPath / std::to_string(pid) / "limits").string());
    if (!content) return std::unexpected(content.error());

    // /proc/<pid>/limits uses fixed column widths: %-25s %-20s %-20s %-10s
    constexpr std::size_t softOffset = 26;
    constexpr std::size_t hardOffset = 47;
    constexpr std::size_t unitsOffset = 68;
    constexpr std::size_t nameWidth = 25;
    constexpr std::size_t fieldWidth = 20;

    ResourceLimitInfo info;
    std::istringstream iss{*content};
    std::string line;
    std::getline(iss, line); // skip header
    while (std::getline(iss, line)) {
        if (line.size() < softOffset) continue;
        ResourceLimit rl;
        rl.resource = utils::trim(line.substr(0, nameWidth));
        rl.softLimit = utils::trim(line.size() > softOffset ? line.substr(softOffset, fieldWidth) : "");
        rl.hardLimit = utils::trim(line.size() > hardOffset ? line.substr(hardOffset, fieldWidth) : "");
        rl.units = utils::trim(line.size() > unitsOffset ? line.substr(unitsOffset) : "");
        if (rl.resource.empty()) continue;
        info.limits.push_back(std::move(rl));
    }
    return info;
}

// Implementation of ProcessAnalyzer::getProcessCgroupInfo
utils::Result<CgroupInfo> ProcessAnalyzer::getProcessCgroupInfo(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    auto content = utils::readTextFile((procPath / std::to_string(pid) / "cgroup").string());
    if (!content) return std::unexpected(content.error());

    CgroupInfo info;
    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        // Format: <id>:<controllers>:<path>
        auto firstColon = line.find(':');
        if (firstColon == std::string::npos) continue;
        auto secondColon = line.find(':', firstColon + 1);
        if (secondColon == std::string::npos) continue;

        auto idVal = utils::parseInteger<int>(line.substr(0, firstColon), parseIntegerBase);
        if (!idVal) continue;

        CgroupEntry entry;
        entry.id = *idVal;
        entry.controllers = line.substr(firstColon + 1, secondColon - firstColon - 1);
        entry.path = line.substr(secondColon + 1);
        info.entries.push_back(std::move(entry));
    }
    return info;
}

// Implementation of ProcessAnalyzer::getProcessMemoryMaps
utils::Result<std::vector<MemoryMapInfo>> ProcessAnalyzer::getProcessMemoryMaps(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) return std::unexpected(check.error());

    auto content = utils::readTextFile((procPath / std::to_string(pid) / "maps").string());
    if (!content) return std::unexpected(content.error());

    constexpr int hexBase = 16;
    constexpr int decBase = 10;

    std::vector<MemoryMapInfo> maps;
    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        // Each line: "<start>-<end> <perms> <offset> <dev> <inode>[ <pathname>]"
        std::istringstream ls(line);
        std::string addressRange;
        std::string perms;
        std::string offsetStr;
        std::string dev;
        std::string inodeStr;
        if (!(ls >> addressRange >> perms >> offsetStr >> dev >> inodeStr)) {
            continue; // malformed line
        }

        auto dashPos = addressRange.find('-');
        if (dashPos == std::string::npos) {
            continue;
        }
        auto startVal = utils::parseInteger<uint64_t>(addressRange.substr(0, dashPos), hexBase);
        auto endVal = utils::parseInteger<uint64_t>(addressRange.substr(dashPos + 1), hexBase);
        if (!startVal || !endVal) {
            continue;
        }

        MemoryMapInfo info;
        info.startAddress = *startVal;
        info.endAddress = *endVal;
        info.permissions = perms;
        if (auto off = utils::parseInteger<uint64_t>(offsetStr, hexBase)) {
            info.offset = *off;
        }
        info.device = dev;
        if (auto ino = utils::parseInteger<uint64_t>(inodeStr, decBase)) {
            info.inode = *ino;
        }

        // The remainder of the line is the (optional) pathname or pseudo-path.
        std::string rest;
        std::getline(ls, rest);
        info.pathname = utils::trim(rest);

        maps.push_back(info);
    }
    return maps;
}