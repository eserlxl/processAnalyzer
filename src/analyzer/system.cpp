// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/types.h"
#include "utils/file.h"

#include <string>
#include <sstream>
#include <unistd.h> // For sysconf

// Anonymous namespace for helper functions
namespace {

// Helper to parse /proc/stat for system boot time
utils::Result<long long> parseSystemBootTime(const std::string& statContent) {
    std::istringstream iss(statContent);
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

