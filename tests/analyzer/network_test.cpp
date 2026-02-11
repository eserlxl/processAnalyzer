#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/network_model.h"
#include "utils/test.h"   // For MockProc

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <memory>

// Test suite for ProcessAnalyzer::getNetworkConnections, specifically the substr fix
class GetNetworkConnectionsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    int testPid = 12345;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_network_test"); // Use a unique mock path for this test fixture
        originalProcPath = analyzer.getProcPath(); // Save original for tear down
        analyzer.setProcPath(mockProc->getPath());  // Set analyzer to use mock path

        mockProc->createPidDir(testPid); // Explicitly create the process directory /proc/<pid>
    }

    void TearDown() override {
        mockProc.reset(); // Clean up the mock /proc environment
        analyzer.setProcPath(originalProcPath); // Restore original proc path
    }
};

TEST_F(GetNetworkConnectionsTest, SocketInodeExtractionWithMalformedPaths) {
    // Constants for test
    const int validSocketFd = 1;
    const int malformedNoBracketFd = 2;
    const int malformedIncompleteFd = 3;
    const int malformedEmptyInodeFd = 4;
    const int malformedNonNumericFd = 5;
    const int malformedMissingBracketFd = 6;
    const int notSocketFd = 7;

    // Setup file descriptors for testPid
    mockProc->createProcFdLink(testPid, validSocketFd, "socket:[12345]"); // Valid socket
    mockProc->createProcFdLink(testPid, malformedNoBracketFd, "socket:[");     // Malformed: no closing bracket
    mockProc->createProcFdLink(testPid, malformedIncompleteFd, "socket:[123");   // Malformed: incomplete
    mockProc->createProcFdLink(testPid, malformedEmptyInodeFd, "socket:[]");     // Malformed: empty inode
    mockProc->createProcFdLink(testPid, malformedNonNumericFd, "socket:[abc]");  // Malformed: non-numeric inode
    mockProc->createProcFdLink(testPid, malformedMissingBracketFd, "socket:[12345"); // Malformed: missing ']'
    mockProc->createProcFdLink(testPid, notSocketFd, "something_else"); // Not a socket

    // Create mock /proc/net/tcp and udp files
    mockProc->createDirectoryAt("net"); // Explicitly create /proc/net
    // Inode 12345 should match. Other inodes should not be present.
    std::string mockTcpContent = 
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:1389 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 12345 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:ABCD 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 99999 1 c4f48000 300 0 0 2 -1\n"; // Inode 99999 will not match
    mockProc->createFile("net/tcp", mockTcpContent);
    mockProc->createFile("net/udp", ""); // Empty UDP for simplicity

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value()) << "Error: " << connectionsResult.error().message();
    
    const auto& connections = connectionsResult.value();
    
    // Only the valid socket connection with inode 12345 should be found
    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "TCP");
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1:5001");
    EXPECT_EQ(connections[0].remoteAddress, "*");
    EXPECT_EQ(connections[0].state, "LISTEN");
}

TEST_F(GetNetworkConnectionsTest, NoSocketFiles) {
    mockProc->createDirectoryAt(std::to_string(testPid) + "/fd"); // Ensure fd directory exists
    mockProc->createDirectoryAt("net"); // Explicitly create /proc/net
    mockProc->createFile("net/tcp", "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"); // Header only
    mockProc->createFile("net/udp", ""); // Empty UDP for simplicity

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value()) << "Error: " << connectionsResult.error().message();
    
    const auto& connections = connectionsResult.value();
    
    ASSERT_EQ(connections.size(), 0); // Expect no connections because no socket FDs were created
}

TEST_F(GetNetworkConnectionsTest, ParseIPv6) {
    const int ipv6SocketFd = 10;
    mockProc->createProcFdLink(testPid, ipv6SocketFd, "socket:[12346]");
    
    mockProc->createDirectoryAt("net");
    // IPv6 Loopback [::1]:8080 (1F90 hex)
    // 00000000000000000000000001000000 -> ::1 in little endian 32-bit chunks?
    // Linux stores IPv6 as 4 32-bit integers in host byte order (usually little endian on x86).
    // So ::1 is 00000000:00000000:00000000:00000001 but in /proc/net/tcp6 it is printed as 4 hex integers.
    // If the machine is little endian, ::1 (0:0:0:1) is stored as 0, 0, 0, 0x01000000 (bytes 0,0,0,1 reversed?).
    // Actually, typical linux /proc/net/tcp6 format for ::1 is "00000000000000000000000001000000".
    // 01000000 is 1 in little endian.
    // Let's verify with "00000000000000000000000001000000" which corresponds to ::1.
    // Port 8080 is 1F90.
    
    std::string mockTcp6Content = 
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000000000000000000001000000:1F90 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000  1000        0 12346 1 c4f48000 300 0 0 2 -1\n";
    
    mockProc->createFile("net/tcp6", mockTcp6Content);
    mockProc->createFile("net/tcp", "");
    mockProc->createFile("net/udp", "");

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();
    
    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].protocol, "TCP6");
    EXPECT_EQ(connections[0].localAddress, "::1:8080");
    EXPECT_EQ(connections[0].state, "LISTEN");
}

TEST_F(GetNetworkConnectionsTest, ParseStatesAndRemoteAddress) {
    const int establishedFd = 20;
    const int timeWaitFd = 21;
    mockProc->createProcFdLink(testPid, establishedFd, "socket:[1001]"); // ESTABLISHED
    mockProc->createProcFdLink(testPid, timeWaitFd, "socket:[1002]"); // TIME_WAIT
    
    mockProc->createDirectoryAt("net");
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
    
    // Order depends on parsing order which is usually file order, but let's check content.
    // Connection 1
    bool foundEst = false;
    bool foundTw = false;
    
    for(const auto& conn : connections) {
        if(conn.state == "ESTABLISHED") {
            foundEst = true;
            EXPECT_EQ(conn.localAddress, "127.0.0.1:80");
            EXPECT_EQ(conn.remoteAddress, "127.0.0.1:12345");
        } else if (conn.state == "TIME_WAIT") {
            foundTw = true;
            EXPECT_EQ(conn.localAddress, "127.0.0.1:81");
            EXPECT_EQ(conn.remoteAddress, "*");
        }
    }
    EXPECT_TRUE(foundEst);
    EXPECT_TRUE(foundTw);
}

TEST_F(GetNetworkConnectionsTest, MalformedNetFileLines) {
    const int validFd = 30;
    mockProc->createProcFdLink(testPid, validFd, "socket:[3001]");
    
    mockProc->createDirectoryAt("net");
    // Line 1: Good
    // Line 2: Missing inode
    // Line 3: Garbage
    // Line 4: Good again
    std::string mockTcpContent = 
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0050 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0 3001 1 c4f48000 300 0 0 2 -1\n"
        "   1: 0100007F:0051 00000000:0000 0A 00000000:0000 00:00000000 00000000  1000        0\n" 
        "   garbage line here\n";
    
    mockProc->createFile("net/tcp", mockTcpContent);

    auto connectionsResult = analyzer.getNetworkConnections(testPid);
    ASSERT_TRUE(connectionsResult.has_value());
    const auto& connections = connectionsResult.value();
    
    ASSERT_EQ(connections.size(), 1);
    EXPECT_EQ(connections[0].localAddress, "127.0.0.1:80");
}
