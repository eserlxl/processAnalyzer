// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/test.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <ranges>
#include <filesystem>
#include <thread>
#include <chrono>
#include <fstream>
#include <memory>

namespace {
    namespace fs = std::filesystem;
    constexpr int socketFd = 10;
    // PIDs
    constexpr int initPid = 1;
    constexpr int kthreaddPid = 2;
    constexpr int myAppPid = 3;
    constexpr int zombiePid = 4;
    constexpr int noStatPid = 5;
    constexpr int nonExistentPid = 9999;
    constexpr int processWithParenthesesPid = 6;
    
    // UIDs
    constexpr int rootUid = 0;
    constexpr int testUserUid = 1000;

    // Durations
    constexpr int updateFileDelayMs = 25;
    constexpr int cpuUsageSampleTimeMs = 50;

    // Memory
    constexpr unsigned long minResidentMemoryKb = 40000;
}

class ProcessAnalyzerAdditionalTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_additional_test");
        setupMockProcFiles();
    }

    void setupMockProcFiles() {
        // PID 1: init-like process
        mockProc->createProcFile(initPid, "status", "Name:\tinit\nState:\tS (sleeping)\nPPid:\t0\nUid:\t0\t0\t0\t0\nThreads:\t1\nVmRSS:\t1000 kB\nVmSize:\t4000 kB\n");
        mockProc->createProcFile(initPid, "stat", "1 (init) S 0 1 1 0 -1 4202752 239 0 0 0 10 20 0 0 20 0 1 0 12345 4096000 1000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(initPid, "cmdline", "systemd\0");
        mockProc->createProcFile(initPid, "io", "rchar: 100\nwchar: 200\n");
        mockProc->createSymlink(initPid, "exe", "/sbin/init");
        mockProc->createSymlink(initPid, "cwd", "/");
        mockProc->createProcFile(initPid, "environ", "PATH=/bin:/sbin\0HOME=/\0");

        // PID 3: another process, child of PID 1, running state
        mockProc->createProcFile(myAppPid, "status", "Name:\tmy-app\nState:\tR (running)\nPPid:\t1\nUid:\t1000\t1000\t1000\t1000\nThreads:\t10\nVmRSS:\t50000 kB\nVmSize:\t100000 kB\n");
        mockProc->createProcFile(myAppPid, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 15 25 0 0 20 15 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        mockProc->createProcFile(myAppPid, "cmdline", "/usr/bin/my-app\0--config\0/etc/my-app.conf");
        mockProc->createProcFile(myAppPid, "environ", "PATH=/usr/bin\0USER=testuser\0");
        mockProc->createSymlink(myAppPid, "exe", "/usr/bin/my-app");
        mockProc->createSymlink(myAppPid, "cwd", "/home/testuser");

        // PID 6: process with parentheses in name
        mockProc->createProcFile(processWithParenthesesPid, "status", "Name:\t(my-process)\nState:\tS (sleeping)\nPPid:\t1\nUid:\t0\t0\t0\t0\nThreads:\t1\nVmRSS:\t2000 kB\nVmSize:\t8000 kB\n");
        mockProc->createProcFile(processWithParenthesesPid, "stat", "6 (my-(process)-with-parens) S 1 6 6 0 -1 4202752 239 0 0 0 20 30 0 0 20 0 1 0 34567 8000000 2000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        // Create cmdline with embedded nulls using string literal and explicit length
        // "(my-process)" (12) + "\0" (1) + "arg1" (4) + "\0" (1) = 18
        mockProc->createProcFile(processWithParenthesesPid, "cmdline", std::string("(my-process)\0arg1\0", 18));

        // Stat file for CPU usage tests
        mockProc->createFile("stat", "cpu  1000 200 800 5000 100 0 50 0\ncpu0 500 100 400 2500 50 0 25 0\nintr 12345\ncxt 6789\nprocesses 10000\n");
        // Uptime file
        mockProc->createFile("uptime", "123456.78 98765.43\n");
        
        // Mock /proc/uptime to simulate failure
        // Initially create a valid one, then modify for tests
        // mockProc->createFile("uptime", "123456.78 98765.43\n"); // Duplicate removed
        
        // Mock /proc/net/dev for network interface stats
        mockProc->createDirectoryAt("net");
        mockProc->createFile("net/dev", "Inter-|   Receive                                                |  Transmit\n face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n  eth0: 100000      100    0    0    0     0          0         0   200000      200    0    0    0     0       0          0\n");
    }

    std::unique_ptr<MockProc> mockProc;
};

// Test for robust parsing of /proc/PID/stat with parentheses in name
TEST_F(ProcessAnalyzerAdditionalTest, GetProcessDetails_NameWithParentheses) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto infoOpt = analyzer.getProcessDetails(processWithParenthesesPid);
    ASSERT_TRUE(infoOpt.has_value());
    const auto& info = infoOpt.value();
    EXPECT_EQ(info.pid, processWithParenthesesPid);
    EXPECT_EQ(info.name, "(my-process)"); // Name from status should be correct
    EXPECT_EQ(info.ppid, initPid);
    EXPECT_EQ(info.cmdline, "(my-process) arg1"); // cmdline should also be correct
}

// Test for robust parsing of /proc/PID/task/TID/stat with parentheses in name
TEST_F(ProcessAnalyzerAdditionalTest, GetProcessThreads_NameWithParentheses) {
    // Mock a thread for processWithParenthesesPid
    mockProc->createDirectoryAt(fs::path(std::to_string(processWithParenthesesPid)) / "task" / "101");
    mockProc->createProcFile(processWithParenthesesPid, "task/101/comm", "(thread-name-with-parens)\n");
    mockProc->createProcFile(processWithParenthesesPid, "task/101/stat", "101 (thread-(name)-with-parens) S 6 101 101 0 -1 4202752 0 0 0 0 5 10 0 0 20 0 1 0 12345 0 0 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto threadsResult = analyzer.getProcessThreads(processWithParenthesesPid);
    ASSERT_TRUE(threadsResult.has_value());
    const auto& threads = *threadsResult;
    ASSERT_EQ(threads.size(), 1);
    EXPECT_EQ(threads[0].tid, 101);
    EXPECT_EQ(threads[0].name, "(thread-name-with-parens)");
    EXPECT_EQ(threads[0].state, "S");
    EXPECT_EQ(threads[0].cpuUserTimeTicks, 5);
    EXPECT_EQ(threads[0].cpuKernelTimeTicks, 10);
}

// Test for getSystemBootTimeUnix error propagation (missing uptime file)
TEST_F(ProcessAnalyzerAdditionalTest, GetSystemBootTimeUnix_MissingUptimeFile) {
    mockProc->removeFile("uptime"); // Remove the mock uptime file
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto bootTimeResult = analyzer.getSystemBootTimeUnix(); // Pass procPath directly
    ASSERT_FALSE(bootTimeResult.has_value());
    EXPECT_EQ(bootTimeResult.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

// Test for getProcessCpuUsage error propagation (final getProcessDetails fails)
TEST_F(ProcessAnalyzerAdditionalTest, GetProcessCpuUsage_FinalDetailsFail) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    
    // Initial setup for myAppPid
    mockProc->createProcFile(myAppPid, "status", "Name:\tmy-app\nState:\tR (running)\nPPid:\t1\nUid:\t1000\t1000\t1000\t1000\nThreads:\t10\nVmRSS:\t50000 kB\nVmSize:\t100000 kB\n");
    mockProc->createProcFile(myAppPid, "stat", "3 (my-app) R 1 3 3 0 -1 4202752 239 0 0 0 15 25 0 0 20 15 1 0 23456 102400000 50000 18446744073709551615 1 1 0 0 0 0 0 4096 0 0 0 0 17 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
    // Initial system stat
    mockProc->createFile("stat", "cpu  1000 200 800 5000 100 0 50 0\n");

    // Simulate process disappearing between snapshots by removing its status file
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(updateFileDelayMs));
        mockProc->removeFile(std::to_string(myAppPid) + "/status");
    });

    auto usageResult = analyzer.getProcessCpuUsage(myAppPid, std::chrono::milliseconds(cpuUsageSampleTimeMs));
    t.join();

    ASSERT_FALSE(usageResult.has_value());
    EXPECT_EQ(usageResult.error(), utils::make_error_code(utils::UtilsError::fileNotFound));
}

// Test for matchesFilter with empty process state
TEST_F(ProcessAnalyzerAdditionalTest, MatchesFilter_EmptyProcessState) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    ProcessFilter filter;
    filter.stateFilter = 'R'; // Filter for running processes

    // Create a mock process with an empty state (e.g., malformed status file)
    mockProc->createPidDir(noStatPid);
    mockProc->createProcFile(noStatPid, "status", "Name:\tno-state\nState:\t\nPPid:\t1\nUid:\t1000\n");
    mockProc->createProcFile(noStatPid, "stat", "5 (no-state) S 1 5 5 0 -1 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");

    auto processesResult = analyzer.queryProcesses(filter);
    ASSERT_TRUE(processesResult.has_value());
    const auto& processes = *processesResult;

    // The process with empty state should not be included when filtering for 'R'
    bool foundNoStateProcess = false;
    for (const auto& p : processes) {
        if (p.pid == noStatPid) {
            foundNoStateProcess = true;
            break;
        }
    }
    EXPECT_FALSE(foundNoStateProcess);
}

// Test for getNetworkConnections (implemented behavior)
TEST_F(ProcessAnalyzerAdditionalTest, GetNetworkConnections_Implemented) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    // Test case 1: No open socket FDs for the process
    mockProc->createDirectoryAt(fs::path(std::to_string(myAppPid)) / "fd"); // Ensure fd dir exists but is empty
    auto connectionsResult = analyzer.getNetworkConnections(myAppPid);
    ASSERT_TRUE(connectionsResult.has_value());
    EXPECT_TRUE(connectionsResult->empty()); 

    // Test case 2: Process does not exist
    auto nonExistentConnectionsResult = analyzer.getNetworkConnections(nonExistentPid);
    ASSERT_FALSE(nonExistentConnectionsResult.has_value());
    EXPECT_EQ(nonExistentConnectionsResult.error(), utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));

    // Test case 3: Mock some socket FDs and verify they are found
    mockProc->createProcFdLink(myAppPid, socketFd, "socket:[54321]");
    // Also create dummy /proc/net/tcp,udp files
    mockProc->createFile("net/tcp", "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n   1: 0100007F:13AD 00000000:0000 0A 00000000:00000000 00:00000000 00000000   1000        0 12345 1 0000000000000000 100 0 0 10 0\n");
    // Using the "old" test data here (AC100101), so expect 1.1.16.172 if fully parsed, or just check existence.
    mockProc->createFile("net/tcp6", "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n   0: 00000000000000000000FFFFAC100101:0050 00000000000000000000000000000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 54321 2 0000000000000000 100 0 0 10 0\n");

    auto connectionsWithSocketsResult = analyzer.getNetworkConnections(myAppPid);
    ASSERT_TRUE(connectionsWithSocketsResult.has_value());
    EXPECT_EQ(connectionsWithSocketsResult->size(), 1);
    EXPECT_EQ(connectionsWithSocketsResult->at(0).protocol, "TCP6");
}

// Test to verify getProcessOpenFileDetails correctly handles permission denied for /proc/PID/fd
TEST_F(ProcessAnalyzerAdditionalTest, GetProcessOpenFileDetails_PermissionDenied) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    // Simulate permission denied for /proc/PID/fd by not creating the directory
    // Instead, MockProc's default behavior for missing directories will cause opendir to fail.
    // Or we can explicitly remove the fd directory to test this.
    mockProc->removeDirectoryAt(fs::path(std::to_string(myAppPid)) / "fd");
    
    // We expect the call to opendir(fdPath.c_str()) to fail, leading to analyzerPermissionDenied
    auto fdsResult = analyzer.getProcessOpenFileDetails(myAppPid);
    ASSERT_FALSE(fdsResult.has_value());
    EXPECT_EQ(fdsResult.error(), utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
}

TEST_F(ProcessAnalyzerAdditionalTest, GetPids_IgnoresOutOfRangeNumericDirectoryName) {
    mockProc->createDirectoryAt("999999999999999999999999999999");

    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    auto pidsResult = analyzer.getPids();
    ASSERT_TRUE(pidsResult.has_value());

    for (int pid : *pidsResult) {
        EXPECT_NE(pid, 999999999);
    }
}

TEST_F(ProcessAnalyzerAdditionalTest, StreamPids_IgnoresOutOfRangeNumericDirectoryName) {
    mockProc->createDirectoryAt("999999999999999999999999999999");

    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));
    std::vector<int> seenPids;
    for (int pid : analyzer.streamPids()) {
        seenPids.push_back(pid);
    }

    EXPECT_EQ(seenPids.size(), 3);
    EXPECT_NE(std::ranges::find(seenPids, initPid), seenPids.end());
    EXPECT_NE(std::ranges::find(seenPids, myAppPid), seenPids.end());
    EXPECT_NE(std::ranges::find(seenPids, processWithParenthesesPid), seenPids.end());
}

TEST_F(ProcessAnalyzerAdditionalTest, GetProcessOpenFileDetails_ClassifiesRegularFileDescriptor) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    const fs::path realFile = fs::absolute(fs::path(mockProc->getPath()) / "regular-file.txt");
    {
        std::ofstream out(realFile);
        out << "data";
    }

    mockProc->createProcFdLink(myAppPid, 42, realFile.string());
    auto fdsResult = analyzer.getProcessOpenFileDetails(myAppPid);
    ASSERT_TRUE(fdsResult.has_value());

    auto it = std::ranges::find_if(*fdsResult, [&](const OpenFileDescriptorInfo& info) {
        return info.fd == 42;
    });
    ASSERT_NE(it, fdsResult->end());
    EXPECT_EQ(it->type, OpenFileType::File);
}

TEST_F(ProcessAnalyzerAdditionalTest, GetNetworkConnections_IgnoresOutOfRangeSocketInode) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    mockProc->createProcFdLink(myAppPid, socketFd, "socket:[999999999999999999999999]");
    mockProc->createFile(
        "net/tcp",
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   1: 0100007F:13AD 00000000:0000 0A 00000000:00000000 00:00000000 00000000   1000        0 12345 1 0000000000000000 100 0 0 10 0\n");

    auto connectionsResult = analyzer.getNetworkConnections(myAppPid);
    ASSERT_TRUE(connectionsResult.has_value());
    EXPECT_TRUE(connectionsResult->empty());
}

TEST_F(ProcessAnalyzerAdditionalTest, GetProcessThreads_MissingTaskDirectoryReturnsEmpty) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    mockProc->createPidDir(noStatPid);
    mockProc->createProcFile(noStatPid, "status", "Name:\tno-task\nPPid:\t1\nUid:\t1000\n");
    auto threadsResult = analyzer.getProcessThreads(noStatPid);

    ASSERT_TRUE(threadsResult.has_value());
    EXPECT_TRUE(threadsResult->empty());
}

TEST_F(ProcessAnalyzerAdditionalTest, GetProcessThreads_MalformedThreadStatDefaultsToZeroCpuTicks) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    mockProc->createDirectoryAt(fs::path(std::to_string(myAppPid)) / "task" / "101");
    mockProc->createProcFile(myAppPid, "task/101/comm", "worker\n");
    mockProc->createProcFile(myAppPid, "task/101/stat", "101 (worker) R");

    auto threadsResult = analyzer.getProcessThreads(myAppPid);
    ASSERT_TRUE(threadsResult.has_value());

    auto it = std::ranges::find_if(*threadsResult, [](const ThreadInfo& thread) {
        return thread.tid == 101;
    });
    ASSERT_NE(it, threadsResult->end());
    EXPECT_EQ(it->name, "worker");
    EXPECT_EQ(it->state, "R");
    EXPECT_EQ(it->cpuUserTimeTicks, 0);
    EXPECT_EQ(it->cpuKernelTimeTicks, 0);
}

TEST_F(ProcessAnalyzerAdditionalTest, GetProcessDiskIoUsage_ClampsNegativeRatesToZero) {
    ProcessAnalyzer analyzer(std::filesystem::path(mockProc->getPath()));

    mockProc->createProcFile(myAppPid, "io", "read_bytes: 500\nwrite_bytes: 700\n");
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(updateFileDelayMs));
        mockProc->createProcFile(myAppPid, "io", "read_bytes: 100\nwrite_bytes: 200\n");
    });

    auto usageResult = analyzer.getProcessDiskIoUsage(myAppPid, std::chrono::milliseconds(cpuUsageSampleTimeMs));
    t.join();

    ASSERT_TRUE(usageResult.has_value());
    EXPECT_DOUBLE_EQ(usageResult->readBytesPerSecond, 0.0);
    EXPECT_DOUBLE_EQ(usageResult->writeBytesPerSecond, 0.0);
}
