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

TEST(SystemClockTicksTest, ReturnsPositiveValueOnLinux) {
    auto result = ProcessAnalyzer::getSystemClockTicksPerSecond();
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0L);
}

TEST(SystemCpuStatsTest, ParsesCpuLine) {
    MockProc mockProc("mock_proc_cpu_stats_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    // cpu  user nice system idle iowait irq softirq steal guest guestNice
    mockProc.createFileAt("stat",
        "cpu  100 20 50 800 10 5 3 1 0 0\n"
        "cpu0 100 20 50 800 10 5 3 1 0 0\n"
        "ctxt 12345\n");

    auto result = analyzer.getSystemCpuStats();
    ASSERT_TRUE(result.has_value());
    const SystemCpuStats& stats = result.value();
    EXPECT_EQ(stats.user,    100ULL);
    EXPECT_EQ(stats.nice,    20ULL);
    EXPECT_EQ(stats.system,  50ULL);
    EXPECT_EQ(stats.idle,    800ULL);
    EXPECT_EQ(stats.iowait,  10ULL);
    EXPECT_EQ(stats.irq,     5ULL);
    EXPECT_EQ(stats.softirq, 3ULL);
    EXPECT_EQ(stats.steal,   1ULL);
    EXPECT_EQ(stats.guest,   0ULL);
    EXPECT_EQ(stats.guestNice, 0ULL);
}

TEST(SystemCpuStatsTest, MissingStatReturnsError) {
    MockProc mockProc("mock_proc_cpu_stats_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());

    auto result = analyzer.getSystemCpuStats();
    EXPECT_FALSE(result.has_value());
}

TEST(SystemCpuStatsTest, MalformedCpuLineReturnsError) {
    MockProc mockProc("mock_proc_cpu_stats_malformed_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    // cpu line has only 2 numeric fields instead of the 8 required for parsing.
    mockProc.createFileAt("stat", "cpu  100 20\n");

    auto result = analyzer.getSystemCpuStats();
    EXPECT_FALSE(result.has_value());
}

TEST(SystemCpuStatsTest, StatFileWithNoCpuAggregateLineReturnsError) {
    MockProc mockProc("mock_proc_cpu_stats_no_aggregate_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    // Only per-CPU lines — no "cpu " aggregate line. The loop exits without returning,
    // falling through to the final error return.
    mockProc.createFileAt("stat",
        "cpu0 100 20 50 800 10 5 3 1 0 0\n"
        "cpu1 80 10 40 900 5 2 1 0 0 0\n");

    auto result = analyzer.getSystemCpuStats();
    EXPECT_FALSE(result.has_value());
}

TEST(SystemCpuStatsTest, ParsesCpuLineWithoutOptionalFields) {
    MockProc mockProc("mock_proc_cpu_stats_8field_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    // Exactly 8 numeric fields — no guest/guestNice; the optional reads fail silently.
    mockProc.createFileAt("stat",
        "cpu  200 30 60 900 15 8 4 2\n"
        "ctxt 99\n");

    auto result = analyzer.getSystemCpuStats();
    ASSERT_TRUE(result.has_value());
    const auto& stats = result.value();
    EXPECT_EQ(stats.user,      200ULL);
    EXPECT_EQ(stats.steal,     2ULL);
    EXPECT_EQ(stats.guest,     0ULL);
    EXPECT_EQ(stats.guestNice, 0ULL);
}

TEST(SystemCpuStatsTest, ParsesCpuLineWithNonZeroGuestFields) {
    MockProc mockProc("mock_proc_cpu_stats_guest_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.createFileAt("stat",
        "cpu  100 20 50 800 10 5 3 1 7 4\n"
        "ctxt 42\n");

    auto result = analyzer.getSystemCpuStats();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().guest,     7ULL);
    EXPECT_EQ(result.value().guestNice, 4ULL);
}

TEST(SystemCpuUsageTest, ReturnsPercentageInRange) {
    // Use real /proc so both snapshots read actual CPU data.
    ProcessAnalyzer analyzer("/proc");
    auto result = analyzer.getSystemCpuUsage(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result.value().cpuPercentage, 0.0);
    EXPECT_LE(result.value().cpuPercentage, 100.0);
}

TEST(SystemCpuUsageTest, MissingStatReturnsError) {
    MockProc mockProc("mock_proc_cpu_usage_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    // No stat file — getSystemCpuStats() will fail on the first call.
    auto result = analyzer.getSystemCpuUsage(std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}

TEST(PerCpuUsageTest, ReturnsUsageForEachCoreInRange) {
    ProcessAnalyzer analyzer("/proc");
    auto result = analyzer.getPerCpuUsage(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    const PerCpuUsage& usage = result.value();
    EXPECT_FALSE(usage.cpuUsages.empty());
    for (const auto& core : usage.cpuUsages) {
        EXPECT_GE(core.cpuId, 0);
        EXPECT_GE(core.cpuPercentage, 0.0);
        EXPECT_LE(core.cpuPercentage, 100.0);
    }
}

TEST(PerCpuUsageTest, MissingStatReturnsError) {
    MockProc mockProc("mock_proc_per_cpu_usage_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    auto result = analyzer.getPerCpuUsage(std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}

TEST(SystemCpuUsageTest, ReturnZeroWhenStatUnchanged) {
    MockProc mockProc("mock_proc_cpu_usage_zero_delta_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.createFileAt("stat", "cpu  1000 200 500 8000 100 50 30 10 0 0\n");
    auto result = analyzer.getSystemCpuUsage(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result->cpuPercentage, 0.0);
}

TEST(PerCpuUsageTest, ParsesCoreCountAndIdsFromMockData) {
    MockProc mockProc("mock_proc_per_cpu_parse_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.createFileAt("stat",
        "cpu  200 0 100 1600 0 0 0 0 0 0\n"
        "cpu0 120 0 60 800 0 0 0 0 0 0\n"
        "cpu1 80 0 40 800 0 0 0 0 0 0\n");
    auto result = analyzer.getPerCpuUsage(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->cpuUsages.size(), 2ULL);
    EXPECT_EQ(result->cpuUsages[0].cpuId, 0);
    EXPECT_EQ(result->cpuUsages[1].cpuId, 1);
    EXPECT_DOUBLE_EQ(result->cpuUsages[0].cpuPercentage, 0.0);
    EXPECT_DOUBLE_EQ(result->cpuUsages[1].cpuPercentage, 0.0);
}

TEST(NetworkInterfaceRatesTest, ReturnsFourRates) {
    ProcessAnalyzer analyzer("/proc");
    auto result = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    for (const auto& r : *result) {
        EXPECT_FALSE(r.interfaceName.empty());
        EXPECT_GE(r.rxBytesPerSec,   0.0);
        EXPECT_GE(r.txBytesPerSec,   0.0);
        EXPECT_GE(r.rxPacketsPerSec, 0.0);
        EXPECT_GE(r.txPacketsPerSec, 0.0);
    }
}

TEST(NetworkInterfaceRatesTest, MissingNetDevReturnsError) {
    MockProc mockProc("mock_proc_net_rates_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    auto result = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}
