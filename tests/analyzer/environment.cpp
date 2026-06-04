// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h" // For MockProc

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class GetProcessEnvironmentTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kPid = 6000;
    static constexpr int kAbsentPid = 9999;

    GetProcessEnvironmentTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_environ_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPid).withName("envproc").withParent(1)
            .withEnviron({{"HOME", "/root"}, {"PATH", "/usr/bin"}}).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetProcessEnvironmentTest, ReturnsEnvironmentEntries) {
    auto result = analyzer.getProcessEnvironment(kPid);
    ASSERT_TRUE(result.has_value());
    const auto& env = result.value();

    EXPECT_NE(std::ranges::find(env, "HOME=/root"), env.end());
    EXPECT_NE(std::ranges::find(env, "PATH=/usr/bin"), env.end());
    EXPECT_EQ(env.size(), 2U);
}

TEST_F(GetProcessEnvironmentTest, AbsentPidReturnsError) {
    auto result = analyzer.getProcessEnvironment(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}
