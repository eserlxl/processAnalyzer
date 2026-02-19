// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#pragma once

#include "utils/types.h"
#include <filesystem>
#include <sys/types.h>

namespace Internal {

// Helper functions declarations
utils::Result<void> checkPidPathExistsAndPermissions(const std::filesystem::path& procPath, pid_t pid);

} // namespace Internal
