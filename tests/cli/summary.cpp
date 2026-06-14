// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/summary.h"
#include "analyzer/process_model.h"

#include <string>
#include <vector>

namespace {
ProcessInfo makeProc(const std::string& state, long threads, long long rss, long long vm) {
    ProcessInfo info;
    info.state = state;
    info.threadCount = threads;
    info.residentMemory = rss;
    info.virtualMemory = vm;
    return info;
}
} // namespace

// The rollup totals equal the sum over the input set, and the by-state map
// counts each state.
TEST(SummaryTest, AggregatesCountsAndTotals) {
    const std::vector<ProcessInfo> procs = {
        makeProc("S", 2, 100, 1000),
        makeProc("R", 1, 50, 500),
        makeProc("S", 3, 25, 250),
    };
    const ProcessSummary s = summarizeProcesses(procs);

    EXPECT_EQ(s.processCount, 3U);
    EXPECT_EQ(s.totalThreads, 6);
    EXPECT_EQ(s.totalResidentMemoryKB, 175);
    EXPECT_EQ(s.totalVirtualMemoryKB, 1750);
    EXPECT_EQ(s.countByState.at("S"), 2U);
    EXPECT_EQ(s.countByState.at("R"), 1U);
    EXPECT_EQ(s.zombieCount, 0U);
}

// Zombie processes (state "Z") are counted both in the by-state map and the
// dedicated zombie tally.
TEST(SummaryTest, CountsZombies) {
    const std::vector<ProcessInfo> procs = {
        makeProc("Z", 1, 0, 0),
        makeProc("Z", 1, 0, 0),
        makeProc("S", 1, 0, 0),
    };
    const ProcessSummary s = summarizeProcesses(procs);

    EXPECT_EQ(s.zombieCount, 2U);
    EXPECT_EQ(s.countByState.at("Z"), 2U);
}

// An empty population is all zeros with no states.
TEST(SummaryTest, EmptySetIsAllZero) {
    const ProcessSummary s = summarizeProcesses({});

    EXPECT_EQ(s.processCount, 0U);
    EXPECT_EQ(s.totalThreads, 0);
    EXPECT_EQ(s.totalResidentMemoryKB, 0);
    EXPECT_EQ(s.zombieCount, 0U);
    EXPECT_TRUE(s.countByState.empty());
}

// JSON output is a single well-formed object carrying every aggregate.
TEST(SummaryTest, JsonOutputIsWellFormed) {
    const std::vector<ProcessInfo> procs = {
        makeProc("S", 2, 100, 1000),
        makeProc("R", 1, 50, 500),
    };
    testing::internal::CaptureStdout();
    printProcessSummary(summarizeProcesses(procs), /*asJson=*/true);
    const std::string out = testing::internal::GetCapturedStdout();

    ASSERT_FALSE(out.empty());
    EXPECT_EQ(out.front(), '{');
    EXPECT_EQ(out.back(), '\n');
    EXPECT_NE(out.find("\"process_count\": 2"), std::string::npos);
    EXPECT_NE(out.find("\"by_state\": {"), std::string::npos);
    EXPECT_NE(out.find("\"total_threads\": 3"), std::string::npos);
    EXPECT_NE(out.find("\"total_resident_kb\": 150"), std::string::npos);
    EXPECT_NE(out.find("\"total_virtual_kb\": 1500"), std::string::npos);
    EXPECT_NE(out.find("\"zombie_count\": 0"), std::string::npos);
}

// The human report carries the labelled sections a reader scans for.
TEST(SummaryTest, HumanOutputContainsKeyLabels) {
    const std::vector<ProcessInfo> procs = {
        makeProc("S", 2, 100, 1000),
    };
    testing::internal::CaptureStdout();
    printProcessSummary(summarizeProcesses(procs), /*asJson=*/false);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("Process Summary"), std::string::npos);
    EXPECT_NE(out.find("Total processes:"), std::string::npos);
    EXPECT_NE(out.find("By state:"), std::string::npos);
    EXPECT_NE(out.find("Zombies:"), std::string::npos);
    EXPECT_NE(out.find("Total threads:"), std::string::npos);
}
