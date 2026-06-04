// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h"

#include <filesystem>
#include <memory>
#include <string>

class GetProcessResourceLimitsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kPid = 8100;
    static constexpr int kAbsentPid = 9999;

    GetProcessResourceLimitsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_limits_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPid).withName("limitsproc").withParent(1).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetProcessResourceLimitsTest, ParsesLimitsFile) {
    // Use fixed-column format matching the Linux kernel: %-25s %-20s %-20s %-10s
    std::string limitsContent =
        "Limit                     Soft Limit           Hard Limit           Units     \n"
        "Max cpu time              unlimited            unlimited            seconds   \n"
        "Max file size             1048576              unlimited            bytes     \n";
    mockProc->createFileAt(std::to_string(kPid) + "/limits", limitsContent);

    auto result = analyzer.getProcessResourceLimits(kPid);
    ASSERT_TRUE(result.has_value());
    const ResourceLimitInfo& info = result.value();
    ASSERT_EQ(info.limits.size(), 2U);

    EXPECT_EQ(info.limits[0].resource, "Max cpu time");
    EXPECT_EQ(info.limits[0].softLimit, "unlimited");
    EXPECT_EQ(info.limits[0].hardLimit, "unlimited");
    EXPECT_EQ(info.limits[0].units, "seconds");

    EXPECT_EQ(info.limits[1].resource, "Max file size");
    EXPECT_EQ(info.limits[1].softLimit, "1048576");
    EXPECT_EQ(info.limits[1].hardLimit, "unlimited");
    EXPECT_EQ(info.limits[1].units, "bytes");
}

TEST_F(GetProcessResourceLimitsTest, AbsentPidReturnsError) {
    auto result = analyzer.getProcessResourceLimits(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}

TEST_F(GetProcessResourceLimitsTest, MissingLimitsFileReturnsError) {
    // Process dir exists but no limits file
    auto result = analyzer.getProcessResourceLimits(kPid);
    EXPECT_FALSE(result.has_value());
}
