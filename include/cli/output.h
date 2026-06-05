// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef CLI_OUTPUT_H
#define CLI_OUTPUT_H

#include <vector>
#include <string>
#include <string_view>
#include "analyzer/process_model.h"

// Escape a string for embedding in a JSON string literal (RFC 8259 §7).
std::string jsonEscape(std::string_view value);

std::vector<std::string> getDefaultColumnsForTable(bool fullDetails);
void printProcessTable(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns, bool noTruncateCmdline);
void printProcessCsv(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns);
void printProcessJson(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns);
void printVerticalProcessDetails(const ProcessInfo& info);

#endif // CLI_OUTPUT_H
