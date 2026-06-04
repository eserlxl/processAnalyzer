// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/network_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <filesystem>
#include <memory>
#include <string>

// Test suite for ProcessAnalyzer::getNetworkConnections. The analyzer collects
// the socket inodes referenced by a process's open fds, then matches them
// against the inode column of /proc/net/{tcp,tcp6,udp,udp6}.
class GetNetworkConnectionsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kSocketFd = 10;
    static constexpr int kMatchingPid = 100;
    static constexpr int kNonMatchingPid = 200;
    static constexpr uint16_t kExpectedLocalPort = 8080;  // 0x1F90
    static constexpr uint16_t kExpectedRemotePort = 80;   // 0x0050

    GetNetworkConnectionsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_net_conn_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->createDirectoryAt("net");

        // A process owning a socket fd whose inode (12345) appears in net/tcp.
        mockProc->buildProcess(kMatchingPid).withName("netproc").withParent(1)
            .withFd(kSocketFd, "socket:[12345]").create();
        // A process whose socket inode (99999) is absent from net/tcp.
        mockProc->buildProcess(kNonMatchingPid).withName("lonely").withParent(1)
            .withFd(kSocketFd, "socket:[99999]").create();

        // /proc/net/tcp: a header line followed by one ESTABLISHED connection
        // (local 127.0.0.1:8080, remote 127.0.0.1:80) owned by inode 12345.
        const std::string tcpContent =
            "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
            "   0: 0100007F:1F90 0100007F:0050 01 00000000:00000000 00:00000000 00000000  1000        0 12345 1 0000000000000000 100 0 0 10 0\n";
        mockProc->createFileAt("net/tcp", tcpContent);
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetNetworkConnectionsTest, ParsesMatchingTcpConnection) {
    auto result = analyzer.getNetworkConnections(kMatchingPid);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);

    const NetworkConnection& conn = result.value().front();
    EXPECT_EQ(conn.protocol, "TCP");
    EXPECT_EQ(conn.localAddress, "127.0.0.1");
    EXPECT_EQ(conn.localPort, kExpectedLocalPort);
    EXPECT_EQ(conn.remoteAddress, "127.0.0.1");
    EXPECT_EQ(conn.remotePort, kExpectedRemotePort);
    EXPECT_EQ(conn.state, "ESTABLISHED");
}

TEST_F(GetNetworkConnectionsTest, ReturnsEmptyWhenNoInodeMatches) {
    auto result = analyzer.getNetworkConnections(kNonMatchingPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}
