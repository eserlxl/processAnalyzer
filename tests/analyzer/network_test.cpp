#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/network_model.h"
#include "utils/test.h" // For MockProc

#include <algorithm>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <cstdint>

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

TEST_F(GetNetworkConnectionsTest, SocketInodeExtractionWithMalformedPaths) {
    mockProc->createProcFdLink(testPid, 1, "socket:[12345]");      // Valid
    mockProc->createProcFdLink(testPid, 2, "socket:[");            // Malformed: no closing bracket
    mockProc->createProcFdLink(testPid, 3, "socket:[123");          // Malformed: incomplete
    mockProc->createProcFdLink(testPid, 4, "socket:[]");            // Malformed: empty inode
    mockProc->createProcFdLink(testPid, 5, "socket:[abc]");         // Malformed: non-numeric inode
    mockProc->createProcFdLink(testPid, 6, "socket:[12345");        // Malformed: missing ']'
    mockProc->createProcFdLink(testPid, 7, "something_else");       // Not a socket

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
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1:5001");
    EXPECT_EQ(connections[0].remoteAddress, "*");
    EXPECT_EQ(connections[0].state, "LISTEN");
}

TEST_F(GetNetworkConnectionsTest, NoSocketFilesForProcess) {
    mockProc->createDirectoryAt(std::to_string(testPid) + "/fd");
    mockProc->createFile("net/tcp", "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n");

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    EXPECT_TRUE(connectionsResult.value().empty());
}

TEST_F(GetNetworkConnectionsTest, ParseIPv6) {
    mockProc->createProcFdLink(testPid, 10, "socket:[12346]");
    
    // The kernel formats IPv6 addresses in /proc/net/{tcp6,udp6} as four 32-bit
    // hexadecimal numbers. Each number is in host-byte order (little-endian on x86).
    // The parser needs to correctly reassemble this into a standard IPv6 string.
    // Address ::1 (localhost) is represented as 00...00, 00...00, 00...00, 01...00
    // Port 8080 is 1F90 in hex.
    std::string mockTcp6Content = 
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000000001000000:1F90 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 12346 1 c4f48000 300 0 0 2 -1\n";
    
    mockProc->createFile("net/tcp6", mockTcp6Content);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();
    
    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "TCP6");
    EXPECT_EQ(connections[0].localAddress, "::1:8080");
    EXPECT_EQ(connections[0].state, "LISTEN");
}

TEST_F(GetNetworkConnectionsTest, ParseDifferentIPv6Address) {
    mockProc->createProcFdLink(testPid, 11, "socket:[12347]");

    // Test with a different IPv6 address: 2001:db8::8a2e:370:7334
    // Which is represented in procfs format.
    // 2001:0db8:0000:0000:8a2e:0370:7334
    // Chunks: 0db82001 00000000 03708a2e 73340000 -> Incorrect representation.
    // Let's manually get the correct BE hex string:
    // 20010db80000000000008a2e03707334 -> this is wrong.
    // Let's use `::ffff:127.0.0.1` -> 0000000000000000FFFF00000100007F (LE format in file)
    std::string mockTcp6Content =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0000000000000000FFFF00000100007F:C3B4 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 12347 1 c4f48000 300 0 0 2 -1\n";
    
    mockProc->createFile("net/tcp6", mockTcp6Content);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "TCP6");
    EXPECT_EQ(connections[0].localAddress, "::ffff:127.0.0.1:50100");
    EXPECT_EQ(connections[0].state, "LISTEN");
}


TEST_F(GetNetworkConnectionsTest, ParseStatesAndRemoteAddress) {
    mockProc->createProcFdLink(testPid, 20, "socket:[1001]"); // ESTABLISHED
    mockProc->createProcFdLink(testPid, 21, "socket:[1002]"); // TIME_WAIT
    
    // 01 (ESTABLISHED), 06 (TIME_WAIT)
    // Local: 127.0.0.1:80 (0100007F:0050) Remote: 127.0.0.1:12345 (0100007F:3039)
    // Local: 127.0.0.1:81 (0100007F:0051) Remote: 0.0.0.0:0 (*)
    std::string mockTcpContent = 
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 0100007F:3039 01 00000000:0000 00:00000000 00000000  1000        0 1001 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:0051 00000000:0000 06 00000000:0000 00:00000000 00000000  1000        0 1002 1 c4f48000 300 0 0 2 -1\n";
    
    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();
    
    ASSERT_EQ(connections.size(), 2);

    const auto itEst = std::ranges::find_if(connections, [](const auto& c) {
        return c.state == "ESTABLISHED";
    });
    ASSERT_NE(itEst, connections.end());
    EXPECT_EQ(itEst->localAddress, "127.0.0.1:80");
    EXPECT_EQ(itEst->remoteAddress, "127.0.0.1:12345");

    const auto itTw = std::ranges::find_if(connections, [](const auto& c) {
        return c.state == "TIME_WAIT";
    });
    ASSERT_NE(itTw, connections.end());
    EXPECT_EQ(itTw->localAddress, "127.0.0.1:81");
    EXPECT_EQ(itTw->remoteAddress, "*");
}

TEST_F(GetNetworkConnectionsTest, MalformedNetFileLines) {
    mockProc->createProcFdLink(testPid, 30, "socket:[3001]");
    
    // Line 1: Good, Line 2: Missing inode, Line 3: Garbage
    std::string mockTcpContent = 
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 3001 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:0051 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0\n" 
        "   garbage line here\n";
    
    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    
    ASSERT_EQ(connectionsResult.value().size(), 1);
    EXPECT_EQ(connectionsResult.value()[0].localAddress, "127.0.0.1:80");
}

TEST_F(GetNetworkConnectionsTest, ParseUdpConnection) {
    mockProc->createProcFdLink(testPid, 40, "socket:[4001]");

    std::string mockUdpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0035 00000000:0000 07 00000000:0000 00:00000000 00000000  1000        0 4001 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/udp", mockUdpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "UDP");
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1:53");
    EXPECT_EQ(connections[0].remoteAddress, "*");
    // UDP is stateless. The 'st' column exists but is not used like TCP.
    // The parser should assign a non-TCP state.
    EXPECT_EQ(connections[0].state, "UNKNOWN");
}

TEST_F(GetNetworkConnectionsTest, ParseUdp6Connection) {
    mockProc->createProcFdLink(testPid, 50, "socket:[5001]");

    std::string mockUdp6Content =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000000001000000:0035 00000000000000000000000000000000:0000 07 00000000:00000000 00:00000000 00000000  1000        0 5001 1 c4f48000 300 0 0 2 -1\n";
    mockProc->createFile("net/udp6", mockUdp6Content);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "UDP6");
    EXPECT_EQ(connections[0].localAddress, "::1:53");
    EXPECT_EQ(connections[0].remoteAddress, "*");
    EXPECT_EQ(connections[0].state, "UNKNOWN");
}

class TcpStateTest : public GetNetworkConnectionsTest, public ::testing::WithParamInterface<std::pair<std::string, std::string>> {};

TEST_P(TcpStateTest, ParseAllTcpStates) {
    const auto& [stateHex, expectedState] = GetParam();
    const int inode = 6001;
    
    mockProc->createProcFdLink(testPid, 1, "socket:[" + std::to_string(inode) + "]");

    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 0100007F:3039 " + stateHex + " 00000000:0000 00:00000000 00000000  1000        0 " + std::to_string(inode) + " 1 c4f48000 300 0 0 2 -1\n";
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
        std::make_pair("0C", "UNKNOWN"), // Not a standard TCP state enum value in <net/tcp_states.h>
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

    // The current implementation of readTextFile returns an empty optional on I/O error,
    // which causes parseNetFileHelper to return an empty vector. The overall result
    // is a valid but empty list of connections. This test verifies that behavior.
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(GetNetworkConnectionsTest, NonExistentPid) {
    auto result = analyzer.getNetworkConnections(99999); // A PID that doesn't exist in mock /proc
    ASSERT_FALSE(result.has_value());
    // This error comes from getProcessOpenFileDetails
    EXPECT_EQ(result.error().message(), "Analyzer: Process not found");
}

TEST_F(GetNetworkConnectionsTest, LargeInodeNumber) {
    const uint64_t largeInode = 9223372036854775807ULL; // 2^63 - 1
    mockProc->createProcFdLink(testPid, 80, "socket:[" + std::to_string(largeInode) + "]");

    std::string mockTcpContent =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 " + std::to_string(largeInode) + " 1 c4f48000 300 0 0 2 -1\n";

    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();

    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1:80");
}

TEST_F(GetNetworkConnectionsTest, MultipleConnectionTypes) {
    mockProc->createProcFdLink(testPid, 1, "socket:[101]"); // TCP
    mockProc->createProcFdLink(testPid, 2, "socket:[102]"); // TCP6
    mockProc->createProcFdLink(testPid, 3, "socket:[103]"); // UDP

    std::string mockTcp = "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
                          "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 101 1 c4f48000 300 0 0 2 -1\n";
    std::string mockTcp6 = "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
                           "   0: 00000000000000000000000001000000:1F90 00000000:00000000 0A 00000000:00000000 00:00000000 00000000  1000        0 102 1 c4f48000 300 0 0 2 -1\n";
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
    std::ranges::sort(connections, [](const auto& a, const auto& b) {
        return a.protocol < b.protocol;
    });

    EXPECT_EQ(connections[0].protocol, "TCP");
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1:80");
    EXPECT_EQ(connections[1].protocol, "TCP6");
    EXPECT_EQ(connections[1].localAddress, "::1:8080");
    EXPECT_EQ(connections[2].protocol, "UDP");
    EXPECT_EQ(connections[2].localAddress, "127.0.0.1:53");
}

TEST_F(GetNetworkConnectionsTest, SocketInodeNotFoundInNetFiles) {
    mockProc->createProcFdLink(testPid, 1, "socket:[999]"); // This inode does not exist in the files below

    std::string mockTcp = "sl local remote st ... inode\n0: 0100007F:0050 ... 101\n";
    mockProc->createFile("net/tcp", mockTcp);
    
    auto result = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}
