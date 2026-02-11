// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/analyzer.h"
#include "utils/core.h"
#include <filesystem>

namespace fs = std::filesystem;

ProcessAnalyzer::ProcessAnalyzer(std::filesystem::path procPath) : procPath(std::move(procPath)) {}

void ProcessAnalyzer::setProcPath(const std::filesystem::path& newPath) {
    procPath = newPath;
}

const std::filesystem::path& ProcessAnalyzer::getProcPath() const {
    return procPath;
}

utils::Result<std::vector<int>> ProcessAnalyzer::getPids() const {
    std::vector<int> pids;
    if (!fs::exists(procPath)) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    try {
        for (const auto& entry : fs::directory_iterator(procPath)) {
            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                if (utils::isInteger(filename)) {
                    if (auto parsedPid = utils::parseInteger<int>(filename)) {
                        pids.push_back(*parsedPid);
                    }
                }
            }
        }
    } catch (const fs::filesystem_error&) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
    }
    return pids;
}

std::generator<int> ProcessAnalyzer::streamPids() const {
    if (!fs::exists(procPath)) {
        co_return; 
    }

    for (const auto& entry : fs::directory_iterator(procPath)) {
        if (entry.is_directory()) {
            std::string filename = entry.path().filename().string();
            if (utils::isInteger(filename)) {
                if (auto parsedPid = utils::parseInteger<int>(filename)) {
                    co_yield *parsedPid;
                }
            }
        }
    }
}

