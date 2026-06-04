// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h"

#include <filesystem>
#include <memory>

class GetSystemBootTimeTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr long long kBootTime = 1700000000LL;

    GetSystemBootTimeTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_boot_time_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemBootTimeTest, ParsesBootTimeFromStatFile) {
    mockProc->createFileAt("stat",
        "cpu  100 0 50 800 0 0 0 0 0 0\n"
        "ctxt 54321\n"
        "btime 1700000000\n"
        "processes 678\n");

    auto result = analyzer.getSystemBootTimeUnix();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), kBootTime);
}

TEST_F(GetSystemBootTimeTest, MissingStatFileReturnsError) {
    auto result = analyzer.getSystemBootTimeUnix();
    EXPECT_FALSE(result.has_value());
}

TEST_F(GetSystemBootTimeTest, StatFileWithNoBtimeReturnsError) {
    mockProc->createFileAt("stat",
        "cpu  100 0 50 800 0 0 0 0 0 0\n"
        "intr 1000000 500 200 300\n"
        "ctxt 54321\n"
        "processes 678\n");

    auto result = analyzer.getSystemBootTimeUnix();
    EXPECT_FALSE(result.has_value());
}
