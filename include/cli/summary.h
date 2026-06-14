// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef CLI_SUMMARY_H
#define CLI_SUMMARY_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include "analyzer/process_model.h"

// Aggregate view of a process population — the one-glance triage numbers that
// neither `system` (system-wide metrics) nor `top` (per-process ranking) report.
struct ProcessSummary {
    std::size_t processCount = 0;
    std::map<std::string, std::size_t> countByState; // single-char state -> count, ordered
    std::size_t zombieCount = 0;
    long long totalThreads = 0;
    long long totalResidentMemoryKB = 0;
    long long totalVirtualMemoryKB = 0;
};

// Pure, deterministic rollup over a process set. Totals equal the sum over the
// input; countByState keys are the per-process `state` strings.
[[nodiscard]] ProcessSummary summarizeProcesses(const std::vector<ProcessInfo>& processes);

// Render the summary: a labelled human report, or a single-line JSON object
// when asJson is true (for piping into automation).
void printProcessSummary(const ProcessSummary& summary, bool asJson);

#endif // CLI_SUMMARY_H
