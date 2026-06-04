// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h"

#include <filesystem>
#include <memory>
#include <string>

class GetProcessCgroupInfoTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kPid = 8200;
    static constexpr int kAbsentPid = 9999;

    GetProcessCgroupInfoTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_cgroup_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPid).withName("cgroupproc").withParent(1).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetProcessCgroupInfoTest, ParsesCgroupEntries) {
    mockProc->createFileAt(std::to_string(kPid) + "/cgroup",
        "12:cpu,cpuacct:/docker/abc\n"
        "0::/\n");

    auto result = analyzer.getProcessCgroupInfo(kPid);
    ASSERT_TRUE(result.has_value());
    const CgroupInfo& info = result.value();
    ASSERT_EQ(info.entries.size(), 2U);

    EXPECT_EQ(info.entries[0].id, 12);
    EXPECT_EQ(info.entries[0].controllers, "cpu,cpuacct");
    EXPECT_EQ(info.entries[0].path, "/docker/abc");

    EXPECT_EQ(info.entries[1].id, 0);
    EXPECT_EQ(info.entries[1].controllers, "");
    EXPECT_EQ(info.entries[1].path, "/");
}

TEST_F(GetProcessCgroupInfoTest, AbsentPidReturnsError) {
    auto result = analyzer.getProcessCgroupInfo(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}

TEST_F(GetProcessCgroupInfoTest, MissingCgroupFileReturnsError) {
    // Process dir exists but no cgroup file
    auto result = analyzer.getProcessCgroupInfo(kPid);
    EXPECT_FALSE(result.has_value());
}
