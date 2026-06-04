// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// Test suite for ProcessAnalyzer::queryProcesses (filtering + sorting).
class QueryProcessesTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    // Named so the fixture data reads clearly and avoids magic-number literals.
    static constexpr pid_t kPidAlphaA = 10;
    static constexpr pid_t kPidBravo = 20;
    static constexpr pid_t kPidAlphaB = 30;
    static constexpr pid_t kPidCharlie = 40;
    static constexpr pid_t kRootPpid = 1;
    static constexpr pid_t kCharliePpid = 2;

    QueryProcessesTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_query_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        // Two processes share the name "alpha" (different pids) so a descending
        // sort by name exercises the equal-key path that a strict-weak-ordering
        // violation in the comparator would corrupt.
        mockProc->buildProcess(kPidAlphaA).withName("alpha").withParent(kRootPpid).create();
        mockProc->buildProcess(kPidBravo).withName("bravo").withParent(kRootPpid).create();
        mockProc->buildProcess(kPidAlphaB).withName("alpha").withParent(kRootPpid).create();

        MockProc::ProcStatData sleeping;
        sleeping.state = 'S';
        mockProc->buildProcess(kPidCharlie).withName("charlie").withParent(kCharliePpid).withStat(sleeping).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }

    static std::vector<std::string> names(const std::vector<ProcessInfo>& procs) {
        std::vector<std::string> out;
        out.reserve(procs.size());
        for (const auto& p : procs) {
            out.push_back(p.name);
        }
        return out;
    }
    static std::vector<pid_t> pids(const std::vector<ProcessInfo>& procs) {
        std::vector<pid_t> out;
        out.reserve(procs.size());
        for (const auto& p : procs) {
            out.push_back(p.pid);
        }
        return out;
    }
};

TEST_F(QueryProcessesTest, SortAscendingByPid) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(pids(result.value()),
              (std::vector<pid_t>{kPidAlphaA, kPidBravo, kPidAlphaB, kPidCharlie}));
}

// Regression guard for the descending-sort strict-weak-ordering fix: the
// comparator must order by key (swapping operands for descending) rather than
// negating, so equal-named elements are preserved and correctly ordered.
TEST_F(QueryProcessesTest, SortDescendingByNameWithEqualKeys) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::name, SortOrder::desc);
    ASSERT_TRUE(result.has_value());
    const auto& procs = result.value();
    ASSERT_EQ(procs.size(), 4U);
    EXPECT_EQ(names(procs), (std::vector<std::string>{"charlie", "bravo", "alpha", "alpha"}));

    // Both equal-named processes survive the sort.
    auto sortedPids = pids(procs);
    EXPECT_NE(std::ranges::find(sortedPids, kPidAlphaA), sortedPids.end());
    EXPECT_NE(std::ranges::find(sortedPids, kPidAlphaB), sortedPids.end());
}

TEST_F(QueryProcessesTest, SortAscendingByName) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::name, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(names(result.value()),
              (std::vector<std::string>{"alpha", "alpha", "bravo", "charlie"}));
}

TEST_F(QueryProcessesTest, FilterByNameContains) {
    ProcessFilter filter;
    filter.nameContains = "alpha";
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(pids(result.value()), (std::vector<pid_t>{kPidAlphaA, kPidAlphaB}));
}

TEST_F(QueryProcessesTest, FilterByPpid) {
    ProcessFilter filter;
    filter.ppidFilter = kCharliePpid;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kPidCharlie);
}

TEST_F(QueryProcessesTest, FilterByState) {
    ProcessFilter filter;
    filter.stateFilter = 'S';
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kPidCharlie);
}

// All fixture processes have the default thread count of 1, so the range-filter
// boundaries exercise both branches of applyRangeFilter.
TEST_F(QueryProcessesTest, FilterByMinThreadsInclusive) {
    ProcessFilter filter;
    filter.minThreads = 1;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 4U);
}

TEST_F(QueryProcessesTest, FilterByMinThreadsExcludesAll) {
    ProcessFilter filter;
    filter.minThreads = 2;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(QueryProcessesTest, FilterByMaxThreadsInclusive) {
    ProcessFilter filter;
    filter.maxThreads = 1;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 4U);
}

TEST_F(QueryProcessesTest, SortDescendingByPid) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::desc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(pids(result.value()),
              (std::vector<pid_t>{kPidCharlie, kPidAlphaB, kPidBravo, kPidAlphaA}));
}

TEST_F(QueryProcessesTest, FilterByCmdlineMatchReturnsSubset) {
    ProcessFilter filter;
    filter.cmdlineContains = "alpha";

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(pids(result.value()), (std::vector<pid_t>{kPidAlphaA, kPidAlphaB}));
}

TEST_F(QueryProcessesTest, FilterByCmdlineNoMatchReturnsEmpty) {
    ProcessFilter filter;
    filter.cmdlineContains = "zzz_no_match";

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(QueryProcessesTest, CustomPredicateFiltersProcesses) {
    ProcessFilter filter;
    filter.customPredicate = [](const ProcessInfo& info) {
        return info.pid <= kPidBravo;
    };

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(pids(result.value()), (std::vector<pid_t>{kPidAlphaA, kPidBravo}));
}

// Fixture for filter integration tests requiring specific stat values.
class QueryStatFilterTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr pid_t kLowPid = 600;
    static constexpr pid_t kHighPid = 601;
    // Low process: vsize=10*1024 bytes → 10 KB virtual; priority=5
    static constexpr unsigned long kLowVsize = 10UL * 1024;
    static constexpr long kLowPriority = 5;
    // High process: vsize=100*1024 bytes → 100 KB virtual; priority=15
    static constexpr unsigned long kHighVsize = 100UL * 1024;
    static constexpr long kHighPriority = 15;

    QueryStatFilterTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_stat_filter_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        MockProc::ProcStatData lowStat;
        lowStat.vsize = kLowVsize;
        lowStat.priority = kLowPriority;
        mockProc->buildProcess(kLowPid).withName("low").withParent(1).withStat(lowStat).create();

        MockProc::ProcStatData highStat;
        highStat.vsize = kHighVsize;
        highStat.priority = kHighPriority;
        mockProc->buildProcess(kHighPid).withName("high").withParent(1).withStat(highStat).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(QueryStatFilterTest, FilterByMinVmExcludesSmall) {
    // min VM of 50 KB should exclude kLowPid (10 KB) and keep kHighPid (100 KB).
    constexpr long long kMinVm = 50LL;
    ProcessFilter filter;
    filter.minVirtualMemoryKB = kMinVm;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    std::vector<pid_t> got;
    for (const auto& p : result.value()) got.push_back(p.pid);
    ASSERT_EQ(got.size(), 1U);
    EXPECT_EQ(got.front(), kHighPid);
}

TEST_F(QueryStatFilterTest, FilterByMaxVmExcludesLarge) {
    // max VM of 50 KB should keep kLowPid (10 KB) and exclude kHighPid (100 KB).
    constexpr long long kMaxVm = 50LL;
    ProcessFilter filter;
    filter.maxVirtualMemoryKB = kMaxVm;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    std::vector<pid_t> got;
    for (const auto& p : result.value()) got.push_back(p.pid);
    ASSERT_EQ(got.size(), 1U);
    EXPECT_EQ(got.front(), kLowPid);
}

TEST_F(QueryStatFilterTest, FilterByMinPriorityExcludesLow) {
    // min priority of 10 should exclude kLowPid (priority 5) and keep kHighPid (priority 15).
    constexpr int kMinPriority = 10;
    ProcessFilter filter;
    filter.minPriority = kMinPriority;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    std::vector<pid_t> got;
    for (const auto& p : result.value()) got.push_back(p.pid);
    ASSERT_EQ(got.size(), 1U);
    EXPECT_EQ(got.front(), kHighPid);
}

TEST_F(QueryStatFilterTest, FilterByMaxPriorityExcludesHigh) {
    // max priority of 10 should keep kLowPid (priority 5) and exclude kHighPid (priority 15).
    constexpr int kMaxPriority = 10;
    ProcessFilter filter;
    filter.maxPriority = kMaxPriority;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    std::vector<pid_t> got;
    for (const auto& p : result.value()) got.push_back(p.pid);
    ASSERT_EQ(got.size(), 1U);
    EXPECT_EQ(got.front(), kLowPid);
}

// Exercises the networkConnectionFilter path in queryProcesses: only the
// process whose socket inode appears in /proc/net/tcp with a matching port
// should survive the filter.
class QueryNetworkFilterTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr pid_t kNetPid = 500;
    static constexpr pid_t kSilentPid = 501;
    static constexpr int kSocketFd = 10;
    static constexpr uint16_t kListeningPort = 8080;    // 0x1F90
    static constexpr uint16_t kNonListeningPort = 9999; // no process bound to this

    QueryNetworkFilterTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_query_net_filter");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        mockProc->buildProcess(kNetPid).withName("netproc").withParent(1)
            .withFd(kSocketFd, "socket:[12345]").create();
        mockProc->buildProcess(kSilentPid).withName("silent").withParent(1).create();

        mockProc->createDirectoryAt("net");
        // net/tcp: one LISTEN entry on port 8080 owned by inode 12345
        const std::string tcpContent =
            "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
            "   0: 00000000:1F90 00000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 12345 1 0000000000000000 100 0 0 10 0\n";
        mockProc->createFileAt("net/tcp", tcpContent);
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(QueryNetworkFilterTest, FilterByLocalPortReturnsOnlyMatchingProcess) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.localPort = kListeningPort;
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kNetPid);
}

TEST_F(QueryNetworkFilterTest, FilterByNonMatchingPortReturnsEmpty) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.localPort = kNonListeningPort;
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(QueryNetworkFilterTest, StreamAppliesNetworkFilterMatchingPort) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.localPort = kListeningPort;
    filter.networkConnectionFilter = netCrit;

    std::vector<pid_t> pids;
    for (const ProcessInfo& info : analyzer.streamQueryProcesses(filter)) {
        pids.push_back(info.pid);
    }
    ASSERT_EQ(pids.size(), 1U);
    EXPECT_EQ(pids.front(), kNetPid);
}

TEST_F(QueryNetworkFilterTest, StreamAppliesNetworkFilterNonMatchingPort) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.localPort = kNonListeningPort;
    filter.networkConnectionFilter = netCrit;

    std::vector<pid_t> pids;
    for (const ProcessInfo& info : analyzer.streamQueryProcesses(filter)) {
        pids.push_back(info.pid);
    }
    EXPECT_TRUE(pids.empty());
}
