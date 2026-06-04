// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

class GetSystemInfoTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr double kUptimeSecs = 7200.0;
    static constexpr double kIdleSecs = 100.0;

    GetSystemInfoTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_sysinfo_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemInfoTest, ParsesUptimeKernelAndHostname) {
    mockProc->createUptime(kUptimeSecs, kIdleSecs);
    mockProc->createVersion("Linux version 6.1.0-test (gcc) #1 SMP");
    mockProc->createFileAt("sys/kernel/hostname", "testhost\n");

    auto result = analyzer.getSystemInfo();
    ASSERT_TRUE(result.has_value());
    const SystemInfo& info = result.value();
    EXPECT_EQ(info.uptime, std::chrono::seconds(static_cast<long long>(kUptimeSecs)));
    EXPECT_NE(info.kernelVersion.find("6.1.0"), std::string::npos);
    EXPECT_EQ(info.hostname, "testhost");
}

TEST_F(GetSystemInfoTest, ParsesOsNameFromOsRelease) {
    mockProc->createUptime(kUptimeSecs, kIdleSecs);
    mockProc->createVersion("Linux version 6.1.0");
    mockProc->createFileAt("etc/os-release", "NAME=\"Arch Linux\"\nVERSION=rolling\n");

    auto result = analyzer.getSystemInfo();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().osName, "Arch Linux");
}

TEST_F(GetSystemInfoTest, OsNameIsNonEmptyWhenMockOsReleaseAbsent) {
    // /etc/os-release is read first (production path); the mock path is the fallback.
    // Either way osName is always non-empty (at least the "Linux" sentinel).
    mockProc->createUptime(kUptimeSecs, kIdleSecs);
    mockProc->createVersion("Linux version 6.1.0");

    auto result = analyzer.getSystemInfo();
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().osName.empty());
}

TEST_F(GetSystemInfoTest, MissingUptimeReturnsError) {
    mockProc->createVersion("Linux version 6.1.0");
    auto result = analyzer.getSystemInfo();
    EXPECT_FALSE(result.has_value());
}

TEST_F(GetSystemInfoTest, MissingVersionReturnsError) {
    mockProc->createUptime(kUptimeSecs, kIdleSecs);
    auto result = analyzer.getSystemInfo();
    EXPECT_FALSE(result.has_value());
}
