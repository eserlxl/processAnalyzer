// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "utils/testing_framework.h"
#include "utils/types.h"

#include <sys/resource.h>
#include <filesystem>
#include <memory>
#include <unistd.h>

class SetProcessPriorityTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;
    int originalNice = 0;

    SetProcessPriorityTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_perf_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath("/proc");
        errno = 0;
        originalNice = ::getpriority(PRIO_PROCESS, static_cast<id_t>(::getpid()));
    }

    void TearDown() override {
        ::setpriority(PRIO_PROCESS, static_cast<id_t>(::getpid()), originalNice);
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(SetProcessPriorityTest, SetsPriorityOnSelf) {
    // Raising nice value (lower priority) is always permitted.
    constexpr int kHighNice = 5;
    auto result = analyzer.setProcessPriority(static_cast<int>(::getpid()), kHighNice);
    if (!result.has_value() &&
        result.error() == utils::make_error_code(utils::UtilsError::analyzerPermissionDenied)) {
        GTEST_SKIP() << "setpriority not permitted in this environment";
    }
    EXPECT_TRUE(result.has_value());
}

TEST_F(SetProcessPriorityTest, NonexistentPidReturnsError) {
    analyzer.setProcPath(mockProc->getPath());
    constexpr int kAbsentPid = 99999;
    auto result = analyzer.setProcessPriority(kAbsentPid, 0);
    EXPECT_FALSE(result.has_value());
}

// ── getProcessCpuAffinity ────────────────────────────────────────────────────

TEST(GetProcessCpuAffinityTest, ParsesRange) {
    constexpr int kPid = 5500;

    MockProc mockProc("mock_proc_affinity_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.buildProcess(kPid).withName("affinproc").withParent(1)
        .withStatusField("Cpus_allowed_list", "0-3").create();

    auto result = analyzer.getProcessCpuAffinity(kPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().cpus, (std::vector<int>{0, 1, 2, 3}));
}

TEST(GetProcessCpuAffinityTest, AbsentPidReturnsError) {
    MockProc mockProc("mock_proc_affinity_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());

    constexpr int kAbsentPid = 99999;
    auto result = analyzer.getProcessCpuAffinity(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}

TEST(GetProcessCpuAffinityTest, MissingFieldReturnsParsingError) {
    constexpr int kPid = 5501;

    MockProc mockProc("mock_proc_affinity_nofield_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    // Process exists but status has no Cpus_allowed_list field.
    mockProc.buildProcess(kPid).withName("affinproc2").withParent(1).create();

    auto result = analyzer.getProcessCpuAffinity(kPid);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

TEST(GetProcessCpuAffinityTest, ParsesCommaSeparatedList) {
    constexpr int kPid = 5502;

    MockProc mockProc("mock_proc_affinity_comma_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.buildProcess(kPid).withName("commaproc").withParent(1)
        .withStatusField("Cpus_allowed_list", "0,2,4").create();

    auto result = analyzer.getProcessCpuAffinity(kPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().cpus, (std::vector<int>{0, 2, 4}));
}

// ── setProcessCpuAffinity ────────────────────────────────────────────────────

TEST(SetProcessCpuAffinityTest, SetsSelfAffinityToAllCpus) {
    ProcessAnalyzer analyzer("/proc");
    CpuSet affinity;
    affinity.cpus.push_back(0);

    auto result = analyzer.setProcessCpuAffinity(static_cast<int>(::getpid()), affinity);
    if (!result.has_value() &&
        result.error() == utils::make_error_code(utils::UtilsError::analyzerPermissionDenied)) {
        GTEST_SKIP() << "sched_setaffinity not permitted in this environment";
    }
    EXPECT_TRUE(result.has_value());
}

TEST(SetProcessCpuAffinityTest, NonexistentPidReturnsError) {
    MockProc mockProc("mock_proc_setaffinity_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());

    constexpr int kAbsentPid = 99999;
    CpuSet affinity;
    affinity.cpus.push_back(0);
    auto result = analyzer.setProcessCpuAffinity(kAbsentPid, affinity);
    EXPECT_FALSE(result.has_value());
}

// ── getProcessCpuUsage ────────────────────────────────────────────────────────

TEST(ProcessCpuUsageTest, ReturnsPercentageInRangeForSelf) {
    ProcessAnalyzer analyzer("/proc");
    const auto selfPid = static_cast<int>(::getpid());
    auto result = analyzer.getProcessCpuUsage(selfPid, std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().pid, selfPid);
    EXPECT_GE(result.value().cpuPercentage, 0.0);
    EXPECT_LE(result.value().cpuPercentage, 100.0);
}

TEST(ProcessCpuUsageTest, AbsentPidReturnsError) {
    MockProc mockProc("mock_proc_cpu_usage_pid_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    constexpr int kAbsentPid = 77777;
    auto result = analyzer.getProcessCpuUsage(kAbsentPid, std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}

// ── getProcessDiskIoUsage ─────────────────────────────────────────────────────

TEST(ProcessDiskIoUsageTest, ReturnsNonNegativeRatesForSelf) {
    ProcessAnalyzer analyzer("/proc");
    const auto selfPid = static_cast<int>(::getpid());
    auto result = analyzer.getProcessDiskIoUsage(selfPid, std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().pid, selfPid);
    EXPECT_GE(result.value().readBytesPerSec, 0LL);
    EXPECT_GE(result.value().writeBytesPerSec, 0LL);
}

TEST(ProcessDiskIoUsageTest, AbsentPidReturnsError) {
    MockProc mockProc("mock_proc_disk_io_pid_absent_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    constexpr int kAbsentPid = 77778;
    auto result = analyzer.getProcessDiskIoUsage(kAbsentPid, std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}
