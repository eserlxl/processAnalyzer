// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class StreamQueryProcessesTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kPidAlpha = 100;
    static constexpr int kPidBravo = 200;
    static constexpr int kPidCharlie = 300;

    StreamQueryProcessesTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_streamquery_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPidAlpha).withName("alpha_proc").withParent(1).create();
        mockProc->buildProcess(kPidBravo).withName("bravo_unique").withParent(1).create();
        mockProc->buildProcess(kPidCharlie).withName("charlie_proc").withParent(1).create();
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(StreamQueryProcessesTest, FilterOnlyReturnsMatchingProcesses) {
    ProcessFilter filter;
    filter.nameContains = "bravo";

    std::vector<std::string> names;
    for (const auto& info : analyzer.streamQueryProcesses(filter)) {
        names.push_back(info.name);
    }
    ASSERT_EQ(names.size(), 1U);
    EXPECT_EQ(names[0], "bravo_unique");
}

TEST_F(StreamQueryProcessesTest, NoFilterReturnsAllProcesses) {
    std::vector<int> pids;
    for (const auto& info : analyzer.streamQueryProcesses()) {
        pids.push_back(info.pid);
    }
    std::ranges::sort(pids);
    EXPECT_EQ(pids, (std::vector<int>{kPidAlpha, kPidBravo, kPidCharlie}));
}

TEST_F(StreamQueryProcessesTest, DefaultSortIsAscendingByPid) {
    std::vector<int> pids;
    for (const auto& info : analyzer.streamQueryProcesses()) {
        pids.push_back(info.pid);
    }
    ASSERT_EQ(pids.size(), 3U);
    EXPECT_TRUE(std::ranges::is_sorted(pids));
}

TEST_F(StreamQueryProcessesTest, SortedQueryReturnsCorrectOrder) {
    std::vector<std::string> names;
    for (const auto& info : analyzer.streamQueryProcesses(
             {}, ProcessSortField::name, SortOrder::asc)) {
        names.push_back(info.name);
    }
    ASSERT_EQ(names.size(), 3U);
    EXPECT_TRUE(std::ranges::is_sorted(names));
}

TEST_F(StreamQueryProcessesTest, FilterWithSortReturnsOnlyMatching) {
    ProcessFilter filter;
    filter.nameContains = "proc";

    std::vector<std::string> names;
    for (const auto& info : analyzer.streamQueryProcesses(
             filter, ProcessSortField::name, SortOrder::asc)) {
        names.push_back(info.name);
    }
    ASSERT_EQ(names.size(), 2U);
    EXPECT_EQ(names[0], "alpha_proc");
    EXPECT_EQ(names[1], "charlie_proc");
}

// Exercises the networkConnectionFilter branch inside streamQueryProcesses'
// filter-only (no-sort) path in base.cpp:214-225. Uses a real TCP entry via
// MockProc so that getNetworkConnections returns a match for one process only.
class StreamQueryNetworkFilterTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr pid_t kNetPid = 500;
    static constexpr pid_t kNoNetPid = 501;
    static constexpr int kSocketFd = 10;
    static constexpr uint16_t kLocalPort = 8080; // 0x1F90

    StreamQueryNetworkFilterTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_stream_net_filter_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->createDirectoryAt("net");

        // kNetPid owns a socket whose inode (77001) appears in net/tcp on port 8080.
        mockProc->buildProcess(kNetPid).withName("netproc").withParent(1)
            .withFd(kSocketFd, "socket:[77001]").create();
        // kNoNetPid has no matching socket inode.
        mockProc->buildProcess(kNoNetPid).withName("plain").withParent(1).create();

        const std::string tcpContent =
            "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
            "   0: 0100007F:1F90 00000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 77001 1 0000000000000000 100 0 0 10 0\n";
        mockProc->createFileAt("net/tcp", tcpContent);
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(StreamQueryNetworkFilterTest, StreamNetworkFilterKeepsMatchingProcess) {
    ProcessFilter filter;
    ProcessFilter::NetworkFilterCriteria netCrit;
    netCrit.localPort = kLocalPort;
    filter.networkConnectionFilter = netCrit;

    std::vector<pid_t> pids;
    for (const auto& info : analyzer.streamQueryProcesses(filter)) {
        pids.push_back(info.pid);
    }
    ASSERT_EQ(pids.size(), 1U);
    EXPECT_EQ(pids[0], kNetPid);
}
