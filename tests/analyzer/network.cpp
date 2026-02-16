// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/network_model.h"
#include "analyzer/system_model.h" // For NetworkInterfaceStats
#include "utils/testing_framework.h" // For MockProc

#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <cstdint>

// Define constants for network states used in tests
// These are mock inode values for specific network states for testing purposes.
constexpr uint64_t establishedInode = 1001ULL;
constexpr uint64_t timeWaitInode = 1002ULL;

// Test suite for ProcessAnalyzer::getNetworkConnections
class GetNetworkConnectionsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    int testPid = 12345;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_network_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());

        mockProc->createPidDir(testPid);
        mockProc->createDirectoryAt("net");
        // Create empty net files by default to avoid errors in tests that don't need them.
        mockProc->createFile("net/tcp", "");
        mockProc->createFile("net/tcp6", "");
        mockProc->createFile("net/udp", "");
        mockProc->createFile("net/udp6", "");
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetNetworkConnectionsTest, CorrectlyParsesTcpConnection) {
    constexpr int tcpFd = 10;
    const uint64_t inode = 12345;
    mockProc->createProcFdLink(testPid, tcpFd, "socket:[" + std::to_string(inode) + "]");

    // Local: 127.0.0.1:80 (LE hex: 0100007F:0050)
    // Remote: 192.168.1.10:8080 (LE hex: 0A01A8C0:1F90)
    // State: ESTABLISHED (01)
    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 0A01A8C0:1F90 01 00000000:0000 00:00000000 00000000  1000        0 1000 1 " + std::to_string(inode) + " 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = *connectionsResult;

    ASSERT_EQ(connections.size(), 1);
    const auto& conn = connections[0];
    EXPECT_EQ(conn.protocol, "TCP");
    EXPECT_EQ(conn.localAddress, "127.0.0.1");
    EXPECT_EQ(conn.localPort, 80);
    EXPECT_EQ(conn.remoteAddress, "192.168.1.10");
    EXPECT_EQ(conn.remotePort, 8080);
    EXPECT_EQ(conn.state, "ESTABLISHED");
    EXPECT_EQ(conn.inode, inode);
    EXPECT_EQ(conn.addressFamily, AddressFamily::iPv4);
    EXPECT_EQ(conn.socketType, SocketType::tcp);
}


TEST_F(GetNetworkConnectionsTest, SocketInodeExtractionWithMalformedPaths) {
    constexpr int validFd = 1;
    constexpr int noClosingBracketFd = 2;
    constexpr int incompleteFd = 3;
    constexpr int emptyInodeFd = 4;
    constexpr int nonNumericInodeFd = 5;
    constexpr int missingBracketFd = 6;
    constexpr int notASocketFd = 7;
    mockProc->createProcFdLink(testPid, validFd, "socket:[12345]");      // Valid
    mockProc->createProcFdLink(testPid, noClosingBracketFd, "socket:[");            // Malformed: no closing bracket
    mockProc->createProcFdLink(testPid, incompleteFd, "socket:[123");         // Malformed: incomplete
    mockProc->createProcFdLink(testPid, emptyInodeFd, "socket:[]");           // Malformed: empty inode
    mockProc->createProcFdLink(testPid, nonNumericInodeFd, "socket:[abc]");        // Malformed: non-numeric inode
    mockProc->createProcFdLink(testPid, missingBracketFd, "socket:[12345");       // Malformed: missing ']'
    mockProc->createProcFdLink(testPid, notASocketFd, "something_else");      // Not a socket

    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:1389 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 12345 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:ABCD 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 99999 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());

    const auto& connections = connectionsResult.value();
    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "TCP");
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1");
    EXPECT_EQ(connections[0].localPort, 5001);
    EXPECT_EQ(connections[0].remoteAddress, "*");
    EXPECT_EQ(connections[0].remotePort, 0);
    EXPECT_EQ(connections[0].state, "LISTEN");
    EXPECT_EQ(connections[0].inode, 12345);
}

TEST_F(GetNetworkConnectionsTest, NoSocketFilesForProcess) {
    mockProc->createDirectoryAt(std::to_string(testPid) + "/fd");
    mockProc->createFile("net/tcp", "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n");

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    EXPECT_TRUE(connectionsResult.value().empty());
}

TEST_F(GetNetworkConnectionsTest, ParseLocalhostIPv6) {
    const uint64_t inode = 12346;
    constexpr int tcp6Fd = 10;
    mockProc->createProcFdLink(testPid, tcp6Fd, "socket:[" + std::to_string(inode) + "]");

    // Address ::1 (localhost) is represented as LE hex chunks.
    // Chunks: 00000000 00000000 00000000 01000000
    // After byte swapping each chunk for network order: 00000000 00000000 00000000 00000001
    // This forms the address ::1.
    // Port 8080 is 1F90 in hex.
    std::string mockTcp6Content =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000000001000000:1F90 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 " + std::to_string(inode) + " 1 c4f48000 300 0 0 2 -1\n";

    mockProc->createFile("net/tcp6", mockTcp6Content);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    const auto& conn = connections[0];
    EXPECT_EQ(conn.protocol, "TCP6");
    EXPECT_EQ(conn.localAddress, "::1");
    EXPECT_EQ(conn.localPort, 8080);
    EXPECT_EQ(conn.state, "LISTEN");
    EXPECT_EQ(conn.inode, inode);
    EXPECT_EQ(conn.addressFamily, AddressFamily::iPv6);
    EXPECT_EQ(conn.socketType, SocketType::tcp);
}


TEST_F(GetNetworkConnectionsTest, ParseIPv4MappedIPv6Address) {
    const uint64_t inode = 12347;
    constexpr int mappedIpv6Fd = 11;
    mockProc->createProcFdLink(testPid, mappedIpv6Fd, "socket:[" + std::to_string(inode) + "]");

    // Address ::ffff:127.0.0.1.
    // 127.0.0.1 is 7F.00.00.01. In LE hex word: 0100007F
    // The procfs representation is 00...00 00...00 FFFF0000 0100007F
    // Port 50100 is C3B4 in hex.
    std::string mockTcp6Content =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0000000000000000FFFF00000100007F:C3B4 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 " + std::to_string(inode) + " 1 c4f48000 300 0 0 2 -1\n";

    mockProc->createFile("net/tcp6", mockTcp6Content);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    const auto& conn = connections[0];
    EXPECT_EQ(conn.protocol, "TCP6");
    EXPECT_EQ(conn.localAddress, "::ffff:127.0.0.1");
    EXPECT_EQ(conn.localPort, 50100);
    EXPECT_EQ(conn.state, "LISTEN");
    EXPECT_EQ(conn.inode, inode);
    EXPECT_EQ(conn.addressFamily, AddressFamily::iPv6);
}


TEST_F(GetNetworkConnectionsTest, ParseStatesAndRemoteAddress) {
    constexpr int establishedFd = 20;
    constexpr int timeWaitFd = 21;
    mockProc->createProcFdLink(testPid, establishedFd, "socket:[" + std::to_string(establishedInode) + "]"); // ESTABLISHED
    mockProc->createProcFdLink(testPid, timeWaitFd, "socket:[" + std::to_string(timeWaitInode) + "]"); // TIME_WAIT

    // Local: 127.0.0.1:80 (LE: 0100007F:0050) Remote: 127.0.0.1:12345 (LE: 0100007F:3039)
    // Local: 127.0.0.1:81 (LE: 0100007F:0051) Remote: 0.0.0.0:0 (*)
    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 0100007F:3039 01 00000000:0000 00:00000000 00000000  1000        0 1000 1 " + std::to_string(establishedInode) + " 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:0051 00000000:0000 06 00000000:0000 00:00000000 00000000  1000        0 1000 1 " + std::to_string(timeWaitInode) + " 1 c4f48000 300 0 0 2 -1\n";

    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    auto& connections = *connectionsResult;
    ASSERT_EQ(connections.size(), 2);
    // Sort by inode to ensure consistent order for testing
    std::ranges::sort(connections, {}, &NetworkConnection::inode);


    const auto& connEst = connections[0];
    EXPECT_EQ(connEst.state, "ESTABLISHED");
    EXPECT_EQ(connEst.localAddress, "127.0.0.1");
    EXPECT_EQ(connEst.localPort, 80);
    EXPECT_EQ(connEst.remoteAddress, "127.0.0.1");
    EXPECT_EQ(connEst.remotePort, 12345);
    EXPECT_EQ(connEst.inode, establishedInode);

    const auto& connTw = connections[1];
    EXPECT_EQ(connTw.state, "TIME_WAIT");
    EXPECT_EQ(connTw.localAddress, "127.0.0.1");
    EXPECT_EQ(connTw.localPort, 81);
    EXPECT_EQ(connTw.remoteAddress, "*");
    EXPECT_EQ(connTw.remotePort, 0);
    EXPECT_EQ(connTw.inode, timeWaitInode);
}

TEST_F(GetNetworkConnectionsTest, MalformedNetFileLines) {
    constexpr int malformedFd = 30;
    mockProc->createProcFdLink(testPid, malformedFd, "socket:[3001]");

    // Line 1: Good, Line 2: Missing inode, Line 3: Garbage, Line 4: Bad IP
    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 3001 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:0051 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0\n"
        "   garbage line here\n"
        "   2: ZZZZZZZZ:0052 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 3002\n";

    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());

    ASSERT_EQ(connectionsResult.value().size(), 1);
    EXPECT_EQ(connectionsResult.value()[0].localAddress, "127.0.0.1");
    EXPECT_EQ(connectionsResult.value()[0].localPort, 80);
    EXPECT_EQ(connectionsResult.value()[0].inode, 3001);
}

TEST_F(GetNetworkConnectionsTest, ParseUdpConnection) {
    const uint64_t inode = 4001;
    constexpr int udpFd = 40;
    mockProc->createProcFdLink(testPid, udpFd, "socket:[" + std::to_string(inode) + "]");

    // Local: 127.0.0.1:53 (LE: 0100007F:0035)
    std::string mockUdpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0035 00000000:0000 07 00000000:0000 00:00000000 00000000  1000        0 " + std::to_string(inode) + " 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/udp", mockUdpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    const auto& conn = connections[0];
    EXPECT_EQ(conn.protocol, "UDP");
    EXPECT_EQ(conn.localAddress, "127.0.0.1");
    EXPECT_EQ(conn.localPort, 53);
    EXPECT_EQ(conn.remoteAddress, "*");
    EXPECT_EQ(conn.remotePort, 0);
    EXPECT_EQ(conn.state, "UNKNOWN");
    EXPECT_EQ(conn.inode, inode);
    EXPECT_EQ(conn.addressFamily, AddressFamily::iPv4);
    EXPECT_EQ(conn.socketType, SocketType::udp);
}

TEST_F(GetNetworkConnectionsTest, ParseUdp6Connection) {
    const uint64_t inode = 5001;
    constexpr int udp6Fd = 50;
    mockProc->createProcFdLink(testPid, udp6Fd, "socket:[" + std::to_string(inode) + "]");

    // Address ::1, Port 53 (0035)
    std::string mockUdp6Content =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000000001000000:0035 00000000000000000000000000000000:0000 07 00000000:00000000 00:00000000 00000000  1000        0 " + std::to_string(inode) + " 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/udp6", mockUdp6Content);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    const auto& conn = connections[0];
    EXPECT_EQ(conn.protocol, "UDP6");
    EXPECT_EQ(conn.localAddress, "::1");
    EXPECT_EQ(conn.localPort, 53);
    EXPECT_EQ(conn.remoteAddress, "*");
    EXPECT_EQ(conn.remotePort, 0);
    EXPECT_EQ(conn.state, "UNKNOWN");
    EXPECT_EQ(conn.inode, inode);
    EXPECT_EQ(conn.addressFamily, AddressFamily::iPv6);
    EXPECT_EQ(conn.socketType, SocketType::udp);
}

class TcpStateTest : public GetNetworkConnectionsTest, public ::testing::WithParamInterface<std::pair<std::string, std::string>> {};

TEST_P(TcpStateTest, ParseAllTcpStates) {
    const auto& [stateHex, expectedState] = GetParam();
    // const uint64_t inode = 6001; // This was not used, removed

    mockProc->createProcFdLink(testPid, 1, "socket:[6001]"); // Mock inode for this test

    std::string mockTcpContent =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 0100007F:3039 " + stateHex + " 00000000:0000 00:00000000 00000000  1000        0 1000 1 6001 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].state, expectedState);
}

INSTANTIATE_TEST_SUITE_P(
    AllTcpStates,
    TcpStateTest,
    ::testing::Values(
        std::make_pair("01", "ESTABLISHED"),
        std::make_pair("02", "SYN_SENT"),
        std::make_pair("03", "SYN_RECV"),
        std::make_pair("04", "FIN_WAIT1"),
        std::make_pair("05", "FIN_WAIT2"),
        std::make_pair("06", "TIME_WAIT"),
        std::make_pair("07", "CLOSE"),
        std::make_pair("08", "CLOSE_WAIT"),
        std::make_pair("09", "LAST_ACK"),
        std::make_pair("0A", "LISTEN"),
        std::make_pair("0B", "CLOSING"),
        std::make_pair("0C", "UNKNOWN"), // Not a standard TCP state enum value
        std::make_pair("FF", "UNKNOWN")  // Definitely not a state
));

TEST_F(GetNetworkConnectionsTest, HandlesUnreadableNetFile) {
    mockProc->createProcFdLink(testPid, 1, "socket:[7001]");

    auto tcpPath = std::filesystem::path(mockProc->getPath()) / "net" / "tcp";
    mockProc->createFile("net/tcp", "unimportant content");
    std::filesystem::permissions(tcpPath, std::filesystem::perms::none);

    auto result = analyzer.getNetworkConnections(testPid);

    // After the test, restore permissions so MockProc cleanup can succeed.
    std::filesystem::permissions(tcpPath, std::filesystem::perms::owner_all);

    // readTextFile returns an empty optional on I/O error,
    // which causes parseNetFile to return an empty vector. The overall result
    // is a valid but empty list of connections. This test verifies that behavior.
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(GetNetworkConnectionsTest, NonExistentPid) {
    const int nonExistentPid = 99999;
    auto result = analyzer.getNetworkConnections(nonExistentPid);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().message(), "Analyzer: Process not found");
}

TEST_F(GetNetworkConnectionsTest, LargeInodeNumber) {
    const uint64_t largeInode = 9223372036854775807ULL; // 2^63 - 1
    constexpr int largeInodeFd = 80;
    mockProc->createProcFdLink(testPid, largeInodeFd, "socket:[" + std::to_string(largeInode) + "]");

    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 " + std::to_string(largeInode) + " 1 c4f48000 300 0 0 2 -1\n";

    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1");
    EXPECT_EQ(connections[0].localPort, 80);
    EXPECT_EQ(connections[0].inode, largeInode); 
}

TEST_F(GetNetworkConnectionsTest, MultipleConnectionTypes) {
    constexpr int tcpFd = 101;
    constexpr int tcp6Fd = 102;
    constexpr int udpFd = 103;
    mockProc->createProcFdLink(testPid, tcpFd, "socket:[101]"); // TCP
    mockProc->createProcFdLink(testPid, tcp6Fd, "socket:[102]"); // TCP6
    mockProc->createProcFdLink(testPid, udpFd, "socket:[103]"); // UDP

    std::string mockTcp = "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
                          "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 101 1 c4f48000 300 0 0 2 -1\n";
    std::string mockTcp6 = "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
                           "   0: 00000000000000000000000001000000:1F90 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 102 1 c4f48000 300 0 0 2 -1\n";
    std::string mockUdp = "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
                          "   0: 0100007F:0035 00000000:0000 07 00000000:0000 00:00000000 00000000  1000        0 103 1 c4f48000 300 0 0 2 -1\n";

    mockProc->createFile("net/tcp", mockTcp);
    mockProc->createFile("net/tcp6", mockTcp6);
    mockProc->createFile("net/udp", mockUdp);

    auto result = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(result.has_value());
    auto& connections = result.value();
    ASSERT_EQ(connections.size(), 3);

    // Sort to make checks deterministic
    std::ranges::sort(connections, {}, &NetworkConnection::protocol);

    const auto& tcpConn = connections[0];
    EXPECT_EQ(tcpConn.protocol, "TCP");
    EXPECT_EQ(tcpConn.localAddress, "127.0.0.1");
    EXPECT_EQ(tcpConn.localPort, 80);
    EXPECT_EQ(tcpConn.inode, 101);

    const auto& tcp6Conn = connections[1];
    EXPECT_EQ(tcp6Conn.protocol, "TCP6");
    EXPECT_EQ(tcp6Conn.localAddress, "::1");
    EXPECT_EQ(tcp6Conn.localPort, 8080);
    EXPECT_EQ(tcp6Conn.inode, 102);

    const auto& udpConn = connections[2];
    EXPECT_EQ(udpConn.protocol, "UDP");
    EXPECT_EQ(udpConn.localAddress, "127.0.0.1");
    EXPECT_EQ(udpConn.localPort, 53);
    EXPECT_EQ(udpConn.inode, 103);
}

TEST_F(GetNetworkConnectionsTest, SocketInodeNotFoundInNetFiles) {
    mockProc->createProcFdLink(testPid, 1, "socket:[999]"); // This inode does not exist in the files below

    std::string mockTcp = "sl local_address rem_address st tx_queue rx_queue tr tm->when retrnsmt uid timeout inode\n"
                          "0: 0100007F:0050 00000000:0000 0A ... 101\n";
    mockProc->createFile("net/tcp", mockTcp);

    auto result = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

// Tests for getNetworkInterfaceStats

class GetNetworkInterfaceStatsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_net_stats_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->createDirectoryAt("net");
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetNetworkInterfaceStatsTest, ParsesValidDevFile) {
    std::string mockDevContent = 
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "    lo: 16186026  163821    0    0    0     0          0         0 16186026  163821    0    0    0     0       0          0\n"
        "  eth0: 12345678    1000    1    2    3     4          5         6 87654321    2000    7    8    9    10      11          12\n";
    
    mockProc->createFile("net/dev", mockDevContent);

    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_TRUE(statsResult.has_value());
    const auto& stats = statsResult.value();

    ASSERT_EQ(stats.size(), 2);
    
    // Check lo
    const auto& lo = stats[0];
    EXPECT_EQ(lo.interfaceName, "lo");
    EXPECT_EQ(lo.rxBytes, 16186026);
    EXPECT_EQ(lo.rxPackets, 163821);
    EXPECT_EQ(lo.txBytes, 16186026);
    
    // Check eth0
    const auto& eth0 = stats[1];
    EXPECT_EQ(eth0.interfaceName, "eth0");
    EXPECT_EQ(eth0.rxBytes, 12345678);
    EXPECT_EQ(eth0.rxPackets, 1000);
    EXPECT_EQ(eth0.rxErrors, 1);
    EXPECT_EQ(eth0.rxDropped, 2);
    EXPECT_EQ(eth0.txBytes, 87654321);
    EXPECT_EQ(eth0.txPackets, 2000);
    EXPECT_EQ(eth0.txErrors, 7);
    EXPECT_EQ(eth0.txDropped, 8);
}

TEST_F(GetNetworkInterfaceStatsTest, HandlesMissingFile) {
    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_FALSE(statsResult.has_value());
    EXPECT_EQ(statsResult.error().value(), static_cast<int>(utils::UtilsError::fileNotFound));
}

TEST_F(GetNetworkInterfaceStatsTest, HandlesEmptyFile) {
    mockProc->createFile("net/dev", "");
    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_FALSE(statsResult.has_value());
    // Expect analyzerParsingError because it's empty
    EXPECT_EQ(statsResult.error().value(), static_cast<int>(utils::UtilsError::analyzerParsingError)); 
}
