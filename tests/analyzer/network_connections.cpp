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

// Standalone test for UDP protocol parsing — verifies net/udp is read and that
// the matched connection reports protocol "UDP".
TEST(GetNetworkConnectionsUdp, ParsesMatchingUdpConnection) {
    constexpr int kUdpPid = 300;
    constexpr int kUdpSocketFd = 5;
    constexpr uint16_t kUdpLocalPort = 5353;  // 0x14E9

    MockProc mockProc("mock_proc_net_conn_udp_test");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.createDirectoryAt("net");
    mockProc.buildProcess(kUdpPid).withName("udpproc").withParent(1)
        .withFd(kUdpSocketFd, "socket:[55555]").create();

    // /proc/net/udp: one entry on port 5353 (0x14E9) owned by inode 55555.
    const std::string udpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000:14E9 00000000:0000 07 00000000:00000000 00:00000000 00000000  1000        0 55555 2 0000000000000000\n";
    mockProc.createFileAt("net/udp", udpContent);

    auto result = analyzer.getNetworkConnections(kUdpPid);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().protocol, "UDP");
    EXPECT_EQ(result.value().front().localPort, kUdpLocalPort);
}

// Standalone test for TCP6 protocol — verifies net/tcp6 is read and that the matched
// connection reports protocol "TCP6" with the expected local port.
TEST(GetNetworkConnectionsTcp6, ParsesTcp6Connection) {
    constexpr int kTcp6Pid = 400;
    constexpr int kTcp6SocketFd = 7;
    constexpr uint16_t kTcp6LocalPort = 9000; // 0x2328

    MockProc mockProc("mock_proc_net_conn_tcp6_test");
    ProcessAnalyzer analyzer(mockProc.getPath());

    mockProc.createDirectoryAt("net");
    mockProc.buildProcess(kTcp6Pid).withName("tcp6proc").withParent(1)
        .withFd(kTcp6SocketFd, "socket:[88888]").create();

    // /proc/net/tcp6: ::1:9000 in LISTEN state, inode 88888.
    // IPv6 ::1 encodes as 00000000000000000000000001000000 (4 LE uint32 words).
    const std::string tcp6Content =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000000001000000:2328 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 88888 1 0000000000000000 100 0 0 10 0\n";
    mockProc.createFileAt("net/tcp6", tcp6Content);

    auto result = analyzer.getNetworkConnections(kTcp6Pid);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1U);
    EXPECT_EQ(result.value().front().protocol, "TCP6");
    EXPECT_EQ(result.value().front().localPort, kTcp6LocalPort);
}
