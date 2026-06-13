// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h" // For MockProc
#include "utils/types.h"             // For UtilsError / make_error_code

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// Direct coverage for getProcessDetails parsing and the hierarchy/snapshot API.
class ProcessDetailsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kInitPpid = 1;
    static constexpr int kStandalonePid = 4242;
    static constexpr int kParent = 4300;
    static constexpr int kChild1 = 4301;
    static constexpr int kChild2 = 4302;
    static constexpr int kGrand = 4303;
    static constexpr int kAbsentPid = 9999;
    static constexpr uid_t kUid = 1000;
    static constexpr long long kRssKb = 2048;
    static constexpr long kThreads = 4;

    ProcessDetailsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_details_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        mockProc->buildProcess(kStandalonePid)
            .withName("myproc")
            .withParent(kInitPpid)
            .withCmdline({"myproc", "--flag"})
            .withExe("/usr/bin/myproc")
            .withCwd(std::filesystem::temp_directory_path())
            .withStatusField("Uid", "1000 1000 1000 1000")
            .withStatusField("VmRSS", "2048 kB")
            .withStatusField("Threads", "4")
            .create();

        // Hierarchy: parent -> {child1, child2}; child1 -> grand.
        mockProc->buildProcess(kParent).withName("parent").create();
        mockProc->buildProcess(kChild1).withName("child1").withParent(kParent).create();
        mockProc->buildProcess(kChild2).withName("child2").withParent(kParent).create();
        mockProc->buildProcess(kGrand).withName("grand").withParent(kChild1).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }

    static std::vector<pid_t> sortedPids(const std::vector<ProcessInfo>& procs) {
        std::vector<pid_t> out;
        out.reserve(procs.size());
        for (const auto& p : procs) {
            out.push_back(p.pid);
        }
        std::ranges::sort(out);
        return out;
    }
};

TEST_F(ProcessDetailsTest, GetProcessDetailsParsesFields) {
    auto result = analyzer.getProcessDetails(kStandalonePid);
    ASSERT_TRUE(result.has_value());
    const ProcessInfo& info = result.value();

    EXPECT_EQ(info.pid, kStandalonePid);
    EXPECT_EQ(info.name, "myproc");
    EXPECT_EQ(info.ppid, kInitPpid);
    EXPECT_EQ(info.state, "R"); // default stat state
    EXPECT_EQ(info.uid, kUid);
    EXPECT_EQ(info.residentMemory, kRssKb);
    EXPECT_EQ(info.threadCount, kThreads);
    EXPECT_EQ(info.cmdline, "myproc --flag");
    EXPECT_EQ(info.executablePath, "/usr/bin/myproc");
}

TEST_F(ProcessDetailsTest, GetProcessDetailsAbsentPidReturnsError) {
    auto result = analyzer.getProcessDetails(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProcessDetailsTest, GetProcessDetailsParsesCurrentWorkingDirectory) {
    auto result = analyzer.getProcessDetails(kStandalonePid);
    ASSERT_TRUE(result.has_value());
    // temp_directory_path() may be a symlink; read_symlink returns the raw target.
    EXPECT_EQ(result.value().currentWorkingDirectory,
              std::filesystem::temp_directory_path().string());
}

TEST_F(ProcessDetailsTest, GetChildProcessesReturnsDirectChildren) {
    auto result = analyzer.getChildProcesses(kParent);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(sortedPids(result.value()), (std::vector<pid_t>{kChild1, kChild2}));
}

TEST_F(ProcessDetailsTest, GetParentProcessReturnsParent) {
    auto result = analyzer.getParentProcess(kChild1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().pid, kParent);
}

TEST_F(ProcessDetailsTest, GetParentProcessOfInitChildReturnsError) {
    // kStandalonePid has ppid 1, which has no meaningful parent.
    auto result = analyzer.getParentProcess(kStandalonePid);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProcessDetailsTest, SnapshotReturnsAllProcesses) {
    auto result = analyzer.snapshot();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(sortedPids(result.value()),
              (std::vector<pid_t>{kStandalonePid, kParent, kChild1, kChild2, kGrand}));
}

TEST_F(ProcessDetailsTest, GetAllDescendantProcessesIncludesGrandchild) {
    auto result = analyzer.getAllDescendantProcesses(kParent);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(sortedPids(result.value()), (std::vector<pid_t>{kChild1, kChild2, kGrand}));
}

TEST_F(ProcessDetailsTest, GetChildProcessesReturnsEmptyForLeafProcess) {
    auto result = analyzer.getChildProcesses(kGrand);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ProcessDetailsTest, GetAllDescendantProcessesReturnsEmptyForLeafProcess) {
    auto result = analyzer.getAllDescendantProcesses(kGrand);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(ProcessDetailsTest, GetParentProcessAbsentPidReturnsError) {
    auto result = analyzer.getParentProcess(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}

TEST_F(ProcessDetailsTest, GetChildProcessesAbsentPidReturnsEmpty) {
    auto result = analyzer.getChildProcesses(kAbsentPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST_F(ProcessDetailsTest, GetAllDescendantProcessesAbsentPidReturnsEmpty) {
    auto result = analyzer.getAllDescendantProcesses(kAbsentPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(GetParentProcessTest, KernelThreadPpidZeroReturnsError) {
    constexpr int kKernelPid = 8800;
    MockProc mockProc("mock_proc_kernel_thread_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.buildProcess(kKernelPid).withName("kthread").withParent(0).create();
    auto result = analyzer.getParentProcess(kKernelPid);
    EXPECT_FALSE(result.has_value());
}

TEST(SnapshotTest, EmptyProcReturnsEmptyVector) {
    MockProc mockProc("mock_proc_snapshot_empty_test");
    ProcessAnalyzer analyzer(mockProc.getPath());
    auto result = analyzer.snapshot();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

// parseStatFile() must reject a /proc/[pid]/stat that lacks the comm parentheses
// or is truncated before the fixed numeric field sequence, rather than returning
// a half-populated ProcessInfo. Overwrite the builder's well-formed stat to drive
// each error branch directly.
TEST_F(ProcessDetailsTest, GetProcessDetailsMalformedStatReturnsError) {
    const auto parsingError =
        utils::make_error_code(utils::UtilsError::analyzerParsingError);

    // (a) Valid leading PID but no comm parentheses -> paren-absence branch.
    constexpr int kNoParenPid = 4400;
    mockProc->buildProcess(kNoParenPid).withName("noparen").create();
    mockProc->createFileAt(std::filesystem::path(std::to_string(kNoParenPid)) / "stat",
                           std::to_string(kNoParenPid) + " comm-without-parens R 1\n");
    auto noParen = analyzer.getProcessDetails(kNoParenPid);
    ASSERT_FALSE(noParen.has_value());
    EXPECT_EQ(noParen.error(), parsingError);

    // (b) Valid PID and comm but truncated before the numeric fields -> field
    // read-failure branch.
    constexpr int kTruncatedPid = 4401;
    mockProc->buildProcess(kTruncatedPid).withName("trunc").create();
    mockProc->createFileAt(std::filesystem::path(std::to_string(kTruncatedPid)) / "stat",
                           std::to_string(kTruncatedPid) + " (trunc) S\n");
    auto truncated = analyzer.getProcessDetails(kTruncatedPid);
    ASSERT_FALSE(truncated.has_value());
    EXPECT_EQ(truncated.error(), parsingError);
}

// A process whose comm contains ')' (a real kernel case, e.g. "(sd-pam)") must
// have its name extracted via rfind(')') so the closing parenthesis is the last
// one on the line, not the first nested one.
TEST_F(ProcessDetailsTest, GetProcessDetailsParsesCommWithParentheses) {
    constexpr int kParenPid = 4402;
    mockProc->buildProcess(kParenPid).withName("weird)proc").withParent(kInitPpid).create();
    auto result = analyzer.getProcessDetails(kParenPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().name, "weird)proc");
}
