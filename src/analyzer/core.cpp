// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include <filesystem>

ProcessAnalyzer::ProcessAnalyzer(std::filesystem::path procPath = "/proc") : procPath(std::move(procPath)) {}
