// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h"

#include <filesystem>
#include <memory>

class GetSystemActivityStatsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr unsigned long long kCtxt = 54321;
    static constexpr unsigned long long kProcesses = 678;
    static constexpr unsigned long long kIntrTotal = 1000000;

    GetSystemActivityStatsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_activity_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemActivityStatsTest, ParsesCtxtAndProcesses) {
    MockProc::SystemStatData data;
    data.ctxt = kCtxt;
    data.processes = kProcesses;
    mockProc->createSystemStat(data);

    auto result = analyzer.getSystemActivityStats();
    ASSERT_TRUE(result.has_value());
    const SystemActivityStats& stats = result.value();
    EXPECT_EQ(stats.contextSwitches, kCtxt);
    EXPECT_EQ(stats.processesForked, kProcesses);
}

TEST_F(GetSystemActivityStatsTest, ParsesInterruptsTotal) {
    mockProc->createFileAt("stat",
        "cpu  100 0 50 800 0 0 0 0 0 0\n"
        "intr 1000000 500 200 300\n"
        "ctxt 54321\n"
        "processes 678\n");

    auto result = analyzer.getSystemActivityStats();
    ASSERT_TRUE(result.has_value());
    const SystemActivityStats& stats = result.value();
    EXPECT_EQ(stats.interruptsTotal, kIntrTotal);
    EXPECT_EQ(stats.contextSwitches, kCtxt);
    EXPECT_EQ(stats.processesForked, kProcesses);
}

TEST_F(GetSystemActivityStatsTest, MissingStatReturnsError) {
    auto result = analyzer.getSystemActivityStats();
    EXPECT_FALSE(result.has_value());
}
