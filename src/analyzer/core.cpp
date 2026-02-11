// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"

ProcessAnalyzer::ProcessAnalyzer(std::filesystem::path procPath) : procPath(std::move(procPath)) {}
