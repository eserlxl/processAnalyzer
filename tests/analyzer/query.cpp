// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <algorithm>
#include <filesystem>
#include <memory>
#include <regex>
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

TEST_F(QueryProcessesTest, FilterByNameRegexMatchesSubset) {
    ProcessFilter filter;
    filter.nameRegex = std::regex("^alpha$");

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(pids(result.value()), (std::vector<pid_t>{kPidAlphaA, kPidAlphaB}));
}

TEST_F(QueryProcessesTest, FilterByNameRegexNoMatch) {
    ProcessFilter filter;
    filter.nameRegex = std::regex("^zzz_no_match$");

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

// Exercises the executablePathContains filter.
class QueryExecPathFilterTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr pid_t kServerPid = 700;
    static constexpr pid_t kHelperPid = 701;

    QueryExecPathFilterTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_exec_filter_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        mockProc->buildProcess(kServerPid).withName("server").withParent(1)
            .withExe("/usr/bin/server").create();
        mockProc->buildProcess(kHelperPid).withName("helper").withParent(1)
            .withExe("/usr/lib/helper").create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(QueryExecPathFilterTest, FilterByExecPathMatchReturnsSubset) {
    ProcessFilter filter;
    filter.executablePathContains = "/usr/bin";

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kServerPid);
}

TEST_F(QueryExecPathFilterTest, FilterByExecPathNoMatchReturnsEmpty) {
    ProcessFilter filter;
    filter.executablePathContains = "/opt/zzz";

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
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

TEST_F(QueryNetworkFilterTest, FilterByConnectionStateMatches) {
    // The fixture's net/tcp entry has state 0x0A = LISTEN.
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.state = "LISTEN";
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kNetPid);
}

TEST_F(QueryNetworkFilterTest, FilterByConnectionStateNoMatch) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.state = "ESTABLISHED";
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(QueryNetworkFilterTest, FilterByProtocolTcpMatches) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.protocol = "TCP";
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kNetPid);
}

TEST_F(QueryNetworkFilterTest, FilterByProtocolUdpNoMatch) {
    // The fixture only has a net/tcp entry; no UDP connections exist.
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.protocol = "UDP";
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

// ── Items 1-4: Sort field coverage ─────────────────────────────────────────

// Fixture with two processes that have distinct stat and I/O values so every
// numeric sort comparator can be exercised with a meaningful ordering check.
class QuerySortByStatTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    // kLowPid has smaller values for all numeric fields; kHighPid has larger.
    static constexpr pid_t kLowPid  = 800;
    static constexpr pid_t kHighPid = 801;

    static constexpr long kLowRss   = 1000L;
    static constexpr long kHighRss  = 5000L;
    static constexpr unsigned long kLowVsize  = 10UL * 1024;
    static constexpr unsigned long kHighVsize = 50UL * 1024;
    static constexpr long kLowThreads  = 1L;
    static constexpr long kHighThreads = 4L;
    static constexpr long kLowPriority  = 5L;
    static constexpr long kHighPriority = 15L;
    static constexpr unsigned long kLowUtime  = 100UL;
    static constexpr unsigned long kHighUtime = 500UL;
    static constexpr unsigned long kLowStime  = 50UL;
    static constexpr unsigned long kHighStime = 200UL;
    static constexpr unsigned long kLowReadBytes  = 1024UL;
    static constexpr unsigned long kHighReadBytes = 8192UL;
    static constexpr unsigned long kLowWriteBytes  = 512UL;
    static constexpr unsigned long kHighWriteBytes = 4096UL;

    QuerySortByStatTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_sort_stat_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        MockProc::ProcStatData lowStat;
        lowStat.rss          = kLowRss;
        lowStat.vsize        = kLowVsize;
        lowStat.num_threads  = kLowThreads;
        lowStat.priority     = kLowPriority;
        lowStat.utime        = kLowUtime;
        lowStat.stime        = kLowStime;

        MockProc::ProcStatData highStat;
        highStat.rss         = kHighRss;
        highStat.vsize       = kHighVsize;
        highStat.num_threads = kHighThreads;
        highStat.priority    = kHighPriority;
        highStat.utime       = kHighUtime;
        highStat.stime       = kHighStime;

        MockProc::ProcIoStats lowIo;
        lowIo.readBytes  = kLowReadBytes;
        lowIo.writeBytes = kLowWriteBytes;

        MockProc::ProcIoStats highIo;
        highIo.readBytes  = kHighReadBytes;
        highIo.writeBytes = kHighWriteBytes;

        mockProc->buildProcess(kLowPid).withName("low").withParent(1)
            .withStat(lowStat).withIoStats(lowIo).create();
        mockProc->buildProcess(kHighPid).withName("high").withParent(1)
            .withStat(highStat).withIoStats(highIo).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }

    // Returns the ordered list of pids from a query sorted by the given field.
    std::vector<pid_t> sortedPids(ProcessSortField field, SortOrder order = SortOrder::asc) {
        ProcessFilter filter;
        auto result = analyzer.queryProcesses(filter, field, order);
        EXPECT_TRUE(result.has_value());
        std::vector<pid_t> pids;
        if (result.has_value()) {
            for (const auto& p : result.value()) pids.push_back(p.pid);
        }
        return pids;
    }
};

// Item 1 — ProcessSortField::rss and ::vmsize
TEST_F(QuerySortByStatTest, SortByRssAscending) {
    auto pids = sortedPids(ProcessSortField::rss);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

TEST_F(QuerySortByStatTest, SortByVmsizeAscending) {
    auto pids = sortedPids(ProcessSortField::vmsize);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

// Item 2 — ProcessSortField::threads and ::priority
TEST_F(QuerySortByStatTest, SortByThreadsAscending) {
    auto pids = sortedPids(ProcessSortField::threads);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

TEST_F(QuerySortByStatTest, SortByPriorityAscending) {
    auto pids = sortedPids(ProcessSortField::priority);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

// Item 3 — ProcessSortField::cpuUserTime, ::cpuKernelTime, ::cpuTime
TEST_F(QuerySortByStatTest, SortByCpuUserTimeAscending) {
    auto pids = sortedPids(ProcessSortField::cpuUserTime);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

TEST_F(QuerySortByStatTest, SortByCpuKernelTimeAscending) {
    auto pids = sortedPids(ProcessSortField::cpuKernelTime);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

TEST_F(QuerySortByStatTest, SortByCpuTimeAscending) {
    // cpuTime = utime + stime; low=(100+50)=150, high=(500+200)=700.
    auto pids = sortedPids(ProcessSortField::cpuTime);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

// Item 4 — ProcessSortField::ioReadBytes and ::ioWriteBytes
TEST_F(QuerySortByStatTest, SortByIoReadBytesAscending) {
    auto pids = sortedPids(ProcessSortField::ioReadBytes);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

TEST_F(QuerySortByStatTest, SortByIoWriteBytesAscending) {
    auto pids = sortedPids(ProcessSortField::ioWriteBytes);
    ASSERT_EQ(pids.size(), 2U);
    EXPECT_EQ(pids[0], kLowPid);
    EXPECT_EQ(pids[1], kHighPid);
}

// ── Item 5: Sort by state and ppid ─────────────────────────────────────────

class QuerySortByStateAndPpidTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    // kRunPid has state='R' ppid=1; kSleepPid has state='S' ppid=2.
    // 'R' < 'S' so ascending state order: kRunPid first.
    // ppid 1 < 2 so ascending ppid order: kRunPid first.
    static constexpr pid_t kRunPid   = 810;
    static constexpr pid_t kSleepPid = 811;

    QuerySortByStateAndPpidTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_sort_state_ppid_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        MockProc::ProcStatData runStat;
        runStat.state = 'R';
        runStat.ppid  = 1;
        MockProc::ProcStatData sleepStat;
        sleepStat.state = 'S';
        sleepStat.ppid  = 2;

        mockProc->buildProcess(kRunPid).withName("runner").withParent(1).withStat(runStat).create();
        mockProc->buildProcess(kSleepPid).withName("sleeper").withParent(2).withStat(sleepStat).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(QuerySortByStateAndPpidTest, SortByStateAscending) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::state, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0].pid, kRunPid);   // 'R' < 'S'
    EXPECT_EQ(result.value()[1].pid, kSleepPid);
}

TEST_F(QuerySortByStateAndPpidTest, SortByPpidAscending) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::ppid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0].pid, kRunPid);   // ppid 1 < 2
    EXPECT_EQ(result.value()[1].pid, kSleepPid);
}

// ── Item 6: Sort by executablePath, cmdline, cwd ──────────────────────────

class QuerySortByPathTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    // "alpha" < "beta" so kAlphaPid should come first in ascending order.
    static constexpr pid_t kAlphaPid = 820;
    static constexpr pid_t kBetaPid  = 821;

    QuerySortByPathTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_sort_path_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        mockProc->buildProcess(kAlphaPid).withName("alpha").withParent(1)
            .withExe("/usr/bin/alpha")
            .withCmdline({"/usr/bin/alpha", "--aopt"})
            .withCwd("/tmp/alpha-work")
            .create();
        mockProc->buildProcess(kBetaPid).withName("beta").withParent(1)
            .withExe("/usr/bin/beta")
            .withCmdline({"/usr/bin/beta", "--bopt"})
            .withCwd("/tmp/beta-work")
            .create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(QuerySortByPathTest, SortByExecPathAscending) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::executablePath, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0].pid, kAlphaPid);
    EXPECT_EQ(result.value()[1].pid, kBetaPid);
}

TEST_F(QuerySortByPathTest, SortByCmdlineAscending) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::cmdline, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0].pid, kAlphaPid);
    EXPECT_EQ(result.value()[1].pid, kBetaPid);
}

TEST_F(QuerySortByPathTest, SortByCwdAscending) {
    ProcessFilter filter;
    auto result = analyzer.queryProcesses(filter, ProcessSortField::cwd, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0].pid, kAlphaPid);
    EXPECT_EQ(result.value()[1].pid, kBetaPid);
}

// ── Item 7: cmdlineRegex and executablePathRegex filter tests ──────────────

TEST(QueryCmdlineRegexFilter, FilterByCmdlineRegexMatchesSubset) {
    constexpr pid_t kPidA = 830;
    constexpr pid_t kPidB = 831;
    MockProc mockProc("mock_proc_cmdline_regex_match");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.buildProcess(kPidA).withName("alpha").withParent(1)
        .withCmdline({"/usr/bin/alpha", "--verbose"}).create();
    mockProc.buildProcess(kPidB).withName("beta").withParent(1)
        .withCmdline({"/usr/bin/beta", "--quiet"}).create();

    ProcessFilter filter;
    filter.cmdlineRegex = std::regex("^/usr/bin/alpha");

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kPidA);
}

TEST(QueryCmdlineRegexFilter, FilterByCmdlineRegexNoMatch) {
    constexpr pid_t kPidA = 832;
    constexpr pid_t kPidB = 833;
    MockProc mockProc("mock_proc_cmdline_regex_nomatch");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.buildProcess(kPidA).withName("alpha").withParent(1)
        .withCmdline({"/usr/bin/alpha"}).create();
    mockProc.buildProcess(kPidB).withName("beta").withParent(1)
        .withCmdline({"/usr/bin/beta"}).create();

    ProcessFilter filter;
    filter.cmdlineRegex = std::regex("^/opt/local/");

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST(QueryExecPathRegexFilter, FilterByExecPathRegexMatchesSubset) {
    constexpr pid_t kPidA = 834;
    constexpr pid_t kPidB = 835;
    MockProc mockProc("mock_proc_exepath_regex_match");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.buildProcess(kPidA).withName("svc").withParent(1)
        .withExe("/usr/lib/systemd/systemd").create();
    mockProc.buildProcess(kPidB).withName("usr").withParent(1)
        .withExe("/usr/bin/grep").create();

    ProcessFilter filter;
    filter.executablePathRegex = std::regex("systemd$");

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kPidA);
}

TEST(QueryExecPathRegexFilter, FilterByExecPathRegexNoMatch) {
    constexpr pid_t kPidA = 836;
    constexpr pid_t kPidB = 837;
    MockProc mockProc("mock_proc_exepath_regex_nomatch");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.buildProcess(kPidA).withName("svc").withParent(1)
        .withExe("/usr/lib/systemd/systemd").create();
    mockProc.buildProcess(kPidB).withName("usr").withParent(1)
        .withExe("/usr/bin/grep").create();

    ProcessFilter filter;
    filter.executablePathRegex = std::regex("^/opt/");

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

// ── Item 8: remotePort and remoteAddressContains network filter tests ───────
//
// The fixture has two processes:
//   kListenPid — LISTEN on 0.0.0.0:8080 (state 0A), inode 22001
//   kEstabPid  — ESTABLISHED from 0.0.0.0:8080 to 10.2.3.4:443 (state 01), inode 22002
//
// IPv4 hex encoding (little-endian 32-bit): 10.2.3.4 → 0403020A, port 443 → 01BB.

class QueryNetworkRemoteFilterTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr pid_t kListenPid = 840;
    static constexpr pid_t kEstabPid  = 841;
    static constexpr uint16_t kRemotePort = 443;
    static constexpr uint16_t kUnusedPort = 80;
    static constexpr int kSocketFdRemote = 10;

    QueryNetworkRemoteFilterTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_query_net_remote");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        mockProc->buildProcess(kListenPid).withName("listener").withParent(1)
            .withFd(kSocketFdRemote, "socket:[22001]").create();
        mockProc->buildProcess(kEstabPid).withName("connected").withParent(1)
            .withFd(kSocketFdRemote, "socket:[22002]").create();

        mockProc->createDirectoryAt("net");
        // net/tcp: LISTEN (inode 22001) + ESTABLISHED to 10.2.3.4:443 (inode 22002).
        const std::string tcpContent =
            "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
            "   0: 00000000:1F90 00000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 22001 1 0000000000000000 100 0 0 10 0\n"
            "   1: 00000000:1F90 0403020A:01BB 01 00000000:00000000 00:00000000 00000000  1000        0 22002 1 0000000000000000 100 0 0 10 0\n";
        mockProc->createFileAt("net/tcp", tcpContent);
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(QueryNetworkRemoteFilterTest, FilterByRemotePortMatches) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.remotePort = kRemotePort;
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kEstabPid);
}

TEST_F(QueryNetworkRemoteFilterTest, FilterByRemotePortNoMatch) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.remotePort = kUnusedPort;
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(QueryNetworkRemoteFilterTest, FilterByRemoteAddressContainsMatches) {
    // The ESTABLISHED entry has remoteAddress "10.2.3.4"; substring "10.2" matches it.
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.remoteAddressContains = "10.2";
    filter.networkConnectionFilter = netCrit;

    auto result = analyzer.queryProcesses(filter, ProcessSortField::pid, SortOrder::asc);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().pid, kEstabPid);
}