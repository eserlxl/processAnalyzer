// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#pragma once

#include "analyzer/core.h"
#include <string>
#include <vector>
#include <filesystem>

namespace Internal {

// Helper functions declarations
utils::Result<unsigned long long> getTotalSystemCpuTimeTicks();
bool matchesFilter(const ProcessInfo& process, const ProcessFilter& filter);
int compareProcesses(const ProcessInfo& a, const ProcessInfo& b, ProcessSortField sortBy);
utils::Result<void> checkPidPathExistsAndPermissions(const std::filesystem::path& procPath, pid_t pid);
utils::Result<std::vector<std::string>> readProcessEnvironmentVars(const std::filesystem::path& procPath, pid_t pid);

} // namespace Internal

