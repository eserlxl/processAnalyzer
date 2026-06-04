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

TEST(GetProcessEnvironmentEdgeCases, EntryWithEqualsInValuePreserved) {
    constexpr int kPid = 6100;
    MockProc mockProc("mock_proc_environ_equals_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.buildProcess(kPid).withName("proc").withParent(1).create();

    // Environ file with a value that contains '=': PATH=/usr/bin:/bin
    std::string content = "PATH=/usr/bin:/bin";
    content += '\0';
    mockProc.createFileAt(std::to_string(kPid) + "/environ", content);

    auto result = analyzer.getProcessEnvironment(kPid);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front(), "PATH=/usr/bin:/bin");
}

TEST(GetProcessEnvironmentEdgeCases, EmptyEnvironReturnsEmptyVector) {
    constexpr int kPid = 6200;
    MockProc mockProc("mock_proc_environ_empty_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.buildProcess(kPid).withName("proc").withParent(1).create();
    mockProc.createFileAt(std::to_string(kPid) + "/environ", "");

    auto result = analyzer.getProcessEnvironment(kPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}
