
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/types.h"
#include "utils/file.h"

#include <string>
#include <sstream>
#include <unistd.h> // For sysconf

// Anonymous namespace for helper functions
namespace {

// Helper to parse /proc/stat for system boot time
utils::Result<long long> parseSystemBootTime(std::string_view statContent) {
    std::istringstream iss{std::string(statContent)};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("btime ")) {
            std::istringstream lineStream(line);
            std::string label; // "btime"
            long long bootTimeUnix;
            if (!(lineStream >> label >> bootTimeUnix)) {
                return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
            }
            return bootTimeUnix;
        }
    }
    return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

} // anonymous namespace

// Implementation of ProcessAnalyzer::getSystemBootTimeUnix
utils::Result<long long> ProcessAnalyzer::getSystemBootTimeUnix() {
    auto statContent = utils::readTextFile("/proc/stat");
    if (!statContent) {
        return std::unexpected(statContent.error());
    }
    return parseSystemBootTime(*statContent);
}

// Implementation of ProcessAnalyzer::getSystemClockTicksPerSecond
utils::Result<long> ProcessAnalyzer::getSystemClockTicksPerSecond() {
    long ticks = sysconf(_SC_CLK_TCK);
    if (ticks == -1) {
        return std::unexpected(std::error_code(errno, std::system_category()));
    }
    return ticks;
}

// Implementation of ProcessAnalyzer::getSystemMemoryInfo
utils::Result<SystemMemoryInfo> ProcessAnalyzer::getSystemMemoryInfo() const {
    auto content = utils::readTextFile((procPath / "meminfo").string());
    if (!content) {
        return std::unexpected(content.error());
    }

    SystemMemoryInfo info;
    // Each /proc/meminfo line is "<Label>: <value> kB"; pull out the value.
    auto readKb = [](const std::string& line, unsigned long& out) {
        std::istringstream ls(line);
        std::string label;
        unsigned long value = 0;
        std::string unit;
        if (ls >> label >> value >> unit) {
            out = value;
        }
    };

    std::istringstream iss{*content};
    std::string line;
    while (std::getline(iss, line)) {
        if (line.starts_with("MemTotal:")) {
            readKb(line, info.memTotal);
        } else if (line.starts_with("MemFree:")) {
            readKb(line, info.memFree);
        } else if (line.starts_with("MemAvailable:")) {
            readKb(line, info.memAvailable);
        } else if (line.starts_with("Buffers:")) {
            readKb(line, info.buffers);
        } else if (line.starts_with("Cached:")) { // not "SwapCached:"
            readKb(line, info.cached);
        } else if (line.starts_with("SwapTotal:")) {
            readKb(line, info.swapTotal);
        } else if (line.starts_with("SwapFree:")) {
            readKb(line, info.swapFree);
        }
    }
    return info;
}

