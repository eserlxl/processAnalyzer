// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h"

#include <filesystem>
#include <memory>

class GetSystemLoadAverageTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    GetSystemLoadAverageTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_loadavg_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemLoadAverageTest, ParsesLoadAverageFields) {
    mockProc->createFileAt("loadavg", "0.52 1.23 0.87 1/450 12345\n");

    auto result = analyzer.getSystemLoadAverage();
    ASSERT_TRUE(result.has_value());
    const SystemLoadAverage& avg = result.value();
    EXPECT_DOUBLE_EQ(avg.oneMin, 0.52);
    EXPECT_DOUBLE_EQ(avg.fiveMin, 1.23);
    EXPECT_DOUBLE_EQ(avg.fifteenMin, 0.87);
}

TEST_F(GetSystemLoadAverageTest, MissingLoadavgReturnsError) {
    auto result = analyzer.getSystemLoadAverage();
    EXPECT_FALSE(result.has_value());
}

TEST_F(GetSystemLoadAverageTest, MalformedLoadavgReturnsError) {
    mockProc->createFileAt("loadavg", "not a number\n");
    auto result = analyzer.getSystemLoadAverage();
    EXPECT_FALSE(result.has_value());
}
